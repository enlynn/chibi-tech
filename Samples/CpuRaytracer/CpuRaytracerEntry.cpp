#include <Game.h>
#include <Engine.h>

#include <Platform/Console.h>
#include <Platform/Assert.h>
#include <Platform/Timer.h>

#include <Systems/ResourceSystem.h>

#include <Math/Math.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <Stb/stb_image_write.h>

#include "Gpu/GpuBuffer.h"
#include "Gpu/GpuRootSignature.h"
#include "Gpu/GpuPso.h"
#include "Gpu/GpuTexture.h"
#include "Gpu/GpuState.h"
#include "Gpu/GpuUtils.h"

#include "Raytracer.h"
#include "WorkQueue.h"

enum class TexRootParamters
{
    Textures = 0,
};

class RaytracerApp : public ct::Game {
public:
    RaytracerApp() : mTaskPool(WorkQueue::getSystemThreadCount() / 2) {}

    [[nodiscard]] ct::GameInfo getGameInfo() const override;

    bool onInit(ct::Engine& tEngine) override;
    bool onUpdate(ct::Engine& tEngine) override;
    bool onRender(ct::Engine& tEngine) override;
    bool onDestroy(ct::Engine& tEngine) override;

    void writeImageToFile(std::string_view tFilename, const void* tData, size_t tWidth, size_t tHeight, size_t tNumChannels, size_t tElementStride);

    static constexpr size_t cBufferedRayTextures = 3;
    static constexpr float  cUploadTimerMS       = 1000.0f;

    std::filesystem::path mOutputPath{};
    WorkQueue             mTaskPool;

    size_t                mRayImageWidth{0};
    size_t                mRayImageHeight{0};

    std::vector<float4>   mImage{};

    GpuTexture            mRayTextures[cBufferedRayTextures]{};
    size_t                mNextRayIndex{0};
    ct::os::Timer         mUploadTimer{};

    GpuBuffer             mIndexResource{};
    GpuRootSignature      mRootSignature{};
    GpuPso                mPso{};

    std::unique_ptr<RaytracerState> mRaytracer{nullptr};
};

std::unique_ptr<ct::Game> onGameLoad() {
    return std::make_unique<RaytracerApp>();
}

ct::GameInfo RaytracerApp::getGameInfo() const {
    return {
            .mWindowTitle = "CPU Raytracer",
            .mAssetPath   = CpuRaytracer_CONTENT_PATH,
    };
}

namespace {
    void copyTextureSubresource(GpuFrameCache& tFrameCache, GpuCommandList& tCommandList,
                                GpuTexture& tTexture, D3D12_SUBRESOURCE_DATA *tSubresources)
    {
        auto dstResource = tTexture.getResource();

        const UINT firstSubresource = 0;
        const UINT numSubresources  = 1;

        tFrameCache.transitionResource(dstResource, D3D12_RESOURCE_STATE_COPY_DEST);
        tFrameCache.flushResourceBarriers(&tCommandList);

        UINT64 requiredSize = getRequiredIntermediateSize(dstResource->asHandle(), firstSubresource, numSubresources);

        CommitedResourceInfo resourceInfo{
                .HeapType     = D3D12_HEAP_TYPE_UPLOAD,
                .Size         = requiredSize,
                .InitialState = D3D12_RESOURCE_STATE_GENERIC_READ,
        };

        GpuResource interimResource = tFrameCache.getDevice()->createCommittedResource(resourceInfo);

        updateSubresources(tCommandList.asHandle(), dstResource->asHandle(), interimResource.asHandle(), 0,
                           firstSubresource, numSubresources, tSubresources);

        tFrameCache.addStaleResource(interimResource);
    }

    void copyRayTextureToGpu(GpuFrameCache& tFrameCache, GpuCommandList& tCommandList, GpuTexture& tTexture,
                             const float4* tPixels, const size_t tWidth, const size_t tHeight) {
        D3D12_SUBRESOURCE_DATA subresource = {};
        subresource.RowPitch   = tWidth * (sizeof(f32) * 4);
        subresource.SlicePitch = subresource.RowPitch * tHeight;
        subresource.pData      = tPixels;

        copyTextureSubresource(tFrameCache, tCommandList, tTexture, &subresource);
    }
}

bool RaytracerApp::onInit(ct::Engine& tEngine) {
    mOutputPath = CpuRaytracer_CONTENT_PATH;
    mOutputPath /= ".cache/results";
    if (!std::filesystem::exists(mOutputPath))
    {
        const bool res = std::filesystem::create_directories(mOutputPath);
        ASSERT(res && std::filesystem::exists(mCompiledShaderDirectory));
    }

    {
        RaytracerInfo info{
            .mImageWidth     = 400,
            .mAspectRatio    = 16.0f / 9.0f,
            .mFocalLength    = 1.0f,
            .mViewportHeight = 2.0f,
            .mCameraOrigin   = {0.0f, 0.0f, 0.0f},
        };

        mRaytracer = std::make_unique<RaytracerState>(info);
    }

    mRayImageWidth  = mRaytracer->mImageWidth;
    mRayImageHeight = mRaytracer->mImageHeight;
    mImage.resize(mRayImageHeight * mRayImageWidth);

    ct::os::Timer queueTimer{};
    queueTimer.start();

    for (int j = 0; j < mRayImageHeight; j++) {
        RaytracerWork work {
            .mImageHeight        = mRayImageHeight,
            .mImageWidth         = mRayImageWidth,
            .mImageHeightIndex   = static_cast<size_t>(j),
            .mImageWriteLocation = mImage.data() + (j * mRayImageWidth),
            .mState              = mRaytracer.get(),
        };

        auto func = static_cast<WorkQueue::TaskFunc>(raytracerWork);
        mTaskPool.addTask(work, func);
    }

    auto timeElapsed = queueTimer.getMilisecondsElapsed();
    ct::console::info("Raytracer Work Queue Time Elapsed %lf miliseconds", timeElapsed);

    ct::os::Timer rayTimer{};
    rayTimer.start();

    mTaskPool.signalThreads();
    mTaskPool.waitForWorkToComplete();

    rayTimer.update();
    timeElapsed = rayTimer.getMilisecondsElapsed();
    ct::console::info("Raytracer Time Elapsed %lf miliseconds", timeElapsed);

    //writeImageToFile("rayimage.png", (void*)mImage.data(), imageWidth, imageHeight, 4, sizeof(ubyte4));

    // Setup rendering
    auto gpuState = tEngine.getGpuState();
    auto frameCache = gpuState->getFrameCache();

    { // Create a root signature
        GpuDescriptorRange diffuseTextureRange{
                .mType               = GpuDescriptorType::Srv,
                .mNumDescriptors     = 1,
                .mBaseShaderRegister = 1,
                .mRegisterSpace      = 1,
                .mDescriptorOffset   = 0, // should this be D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND (-1)?
        };

        GpuDescriptorTable descriptorTables[1]{
                {
                        .mRootIndex        = (u32)TexRootParamters::Textures,
                        .mVisibility       = GpuDescriptorVisibility::Pixel,
                        .mDescriptorRanges = farray(&diffuseTextureRange, 1),
                },
        };

        D3D12_STATIC_SAMPLER_DESC sampler = getStaticSamplerDesc(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
        const char* debugName = "Raytracer Composite RS";

        GpuRootSignatureInfo Info = {};
        Info.mDescriptorTables    = farray(descriptorTables, 1);
        Info.mStaticSamplers      = farray(&sampler, 1);
        Info.mName                = debugName;
        mRootSignature = GpuRootSignature(*frameCache->getDevice(), Info);
    }

    { // Create a pipeline state object
        ct::ShaderResource vertexShader = tEngine.loadShader("FullscreenQuad",     ct::ShaderStage::Vertex);
        ct::ShaderResource pixelShader  = tEngine.loadShader("RaytracerComposite", ct::ShaderStage::Pixel);

        DXGI_FORMAT rtFormat = frameCache->mGlobal->mSwapchain->getSwapchainFormat();

        D3D12_RT_FORMAT_ARRAY psoRTFormats = {};
        psoRTFormats.NumRenderTargets = 1;
        psoRTFormats.RTFormats[0]     = rtFormat;

        // leave msaa disabled
        u32 sampleCount   = 1;
        u32 sampleQuality = 0;

        GpuGraphicsPsoBuilder builder = GpuGraphicsPsoBuilder::builder();
        builder.setRootSignature(&mRootSignature)
                .setVertexShader(vertexShader)
                .setPixelShader(pixelShader)
                .setRenderTargetFormats({rtFormat})
                .setSampleQuality(sampleCount, sampleQuality)
                .setDepthStencilState(getDepthStencilState(GpuDepthStencilState::ReadWrite), DXGI_FORMAT_D32_FLOAT);

        mPso = builder.compile(frameCache);
    }

    { // Create the triangle and upload data
        u16 indices[] = { 0, 1, 2, 1, 2, 3 };

        GpuIndexBufferInfo indexInfo = {
                .mIsU16      = true,
                .mIndexCount = ArrayCount(indices),
                .mIndices    = indices,
        };
        mIndexResource = GpuBuffer::createIndexBuffer(frameCache, indexInfo);
    }

    { // Create the textures.
        for (auto & rayTexture : mRayTextures) {
            const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT; // TODO:

            D3D12_RESOURCE_DESC rsrcDesc = getTex2DDesc(format, mRayImageWidth, mRayImageHeight);
            rayTexture = GpuTexture(frameCache, rsrcDesc);
            copyRayTextureToGpu(*frameCache, *frameCache->borrowCopyCommandList(), mRayTextures[mNextRayIndex], mImage.data(), mRayImageWidth, mRayImageHeight);
        }

        // Copy to the first and second texture.
        mNextRayIndex = (mNextRayIndex + 1) % cBufferedRayTextures;
        mUploadTimer.start();
    }

    frameCache->submitCopyCommandList();
    frameCache->flushGpu(); // Forcibly upload everything (for now)

    return true;
}

bool RaytracerApp::onUpdate(ct::Engine& tEngine) {
    return true;
}

bool RaytracerApp::onRender(ct::Engine& tEngine) {
    auto gpuState = tEngine.getGpuState();

    gpuState->beginFrame();

    auto frameCache = gpuState->getFrameCache();
    GpuCommandList *commandList = frameCache->borrowGraphicsCommandList();

    // Upadte the current Ray texture
    //

//    mUploadTimer.update();
//    auto elapsedMS = mUploadTimer.getMilisecondsElapsed();
//    if (elapsedMS > cUploadTimerMS) {
//        //copyRayTextureToGpu(*frameCache, *commandList, mRayTextures[mNextRayIndex], mImage.data(), mRayImageWidth, mRayImageHeight);
//        mNextRayIndex = (mNextRayIndex + 1) % cBufferedRayTextures;
//        mUploadTimer.start();
//    }

    size_t textureIndex = (mNextRayIndex == 0) ? cBufferedRayTextures - 1 : mNextRayIndex - 1;
    GpuTexture& activeRayTexture = mRayTextures[textureIndex];

    // Render
    //

    GpuRenderTarget renderTarget{};
    { // Setup Render Target
        // Technically don't need the depth buffer, but bind it for demonstration purposes
        GpuTexture *SceneFramebuffer = frameCache->getFramebuffer(GpuFramebufferBinding::MainColor);
        GpuTexture *DepthBuffer = frameCache->getFramebuffer(GpuFramebufferBinding::DepthStencil);

        renderTarget.attachTexture(AttachmentPoint::Color0, SceneFramebuffer);
        renderTarget.attachTexture(AttachmentPoint::DepthStencil, DepthBuffer);

        // Clear the Framebuffer and bind the render target.
        frameCache->transitionResource(SceneFramebuffer->getResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
        frameCache->transitionResource(DepthBuffer->getResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

        float4 ClearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        commandList->bindRenderTarget(&renderTarget, &ClearColor, true);
    }

    { // Scissor / Viewport
        D3D12_VIEWPORT Viewport = renderTarget.getViewport();
        commandList->setViewport(Viewport);

        D3D12_RECT ScissorRect = {};
        ScissorRect.left = 0;
        ScissorRect.top = 0;
        ScissorRect.right = (LONG) Viewport.Width;
        ScissorRect.bottom = (LONG) Viewport.Height;
        commandList->setScissorRect(ScissorRect);
    }

    { // Draw the triangle
        commandList->setPipelineState(mPso);
        commandList->setGraphicsRootSignature(mRootSignature);

        // Bind the texture
        frameCache->transitionResource(activeRayTexture.getResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->setShaderResourceView((u32)TexRootParamters::Textures, 0, activeRayTexture);

        // Draw
        commandList->setTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        commandList->setIndexBuffer(mIndexResource.getIndexBufferView());

        frameCache->flushResourceBarriers(commandList); // flush barriers before drawing
        commandList->drawIndexedInstanced(mIndexResource.getIndexCount());
    }

    { // Copy the Scene Target into the swapchain's render target
        GpuRenderTarget* swapchainRenderTarget = frameCache->mGlobal->mSwapchain->getRenderTarget();

        const GpuTexture* swapchainBackBuffer = swapchainRenderTarget->getTexture(AttachmentPoint::Color0);
        const GpuTexture* sceneTexture        = frameCache->getFramebuffer(GpuFramebufferBinding::MainColor);

        commandList->copyResource(frameCache, swapchainBackBuffer->getResource(), sceneTexture->getResource());

        frameCache->transitionResource(swapchainBackBuffer->getResource(), D3D12_RESOURCE_STATE_PRESENT);
        frameCache->flushResourceBarriers(commandList);
    }

    gpuState->endFrame(); // TODO: endFrame should optionally take the scene framebuffer, and handle MSAA resolve.

    return true;
}

bool RaytracerApp::onDestroy(ct::Engine& tEngine) {
    return true;
}

void RaytracerApp::writeImageToFile(std::string_view tFilename, const void *tData, size_t tWidth, size_t tHeight,
                                    size_t tNumChannels, size_t tElementStride) {

    std::filesystem::path output = mOutputPath / tFilename;
    auto outputStr8 = output.string();

    const int rowStride = tElementStride * tWidth;
    int res = stbi_write_png(outputStr8.c_str(), (int)tWidth, (int)tHeight,(int)tNumChannels, tData, rowStride);
    ASSERT(res != 0);
}

// entry point
#include <Entry.h>