#include <Game.h>
#include <Engine.h>

#include <Platform/Console.h>

#include <Gpu/GpuBuffer.h>
#include <Gpu/GpuState.h>

#include <Systems/ResourceSystem.h>

#include <Math/Geometry.h>

struct VertexDrawConstants
{
    u32 uVertexOffset      = 0;
    u32 uVertexBufferIndex = 0;
    u32 uMeshDataIndex     = 0;
};

class HelloTriangleApp : public ct::Game {
public:
    [[nodiscard]] ct::GameInfo getGameInfo() const override;

    bool onInit(ct::Engine& tEngine) override;
    bool onUpdate(ct::Engine& tEngine) override;
    bool onRender(ct::Engine& tEngine) override;
    bool onDestroy() override;

    ResourceSystem        mResourceSystem{};

    GpuBuffer             mVertexResource{};
    GpuBuffer             mIndexResource{};

    GpuRootSignature      mRootSignature{};
    GpuPso                mPso{};
};

std::unique_ptr<ct::Game> onGameLoad() {
    return std::make_unique<HelloTriangleApp>();
}

ct::GameInfo HelloTriangleApp::getGameInfo() const {
    return {
            .mWindowTitle = "Hello Triangle",
            .mAssetPath   = HelloTriangle_CONTENT_PATH,
    };
}

bool HelloTriangleApp::onInit(ct::Engine& tEngine) {
    auto gpuState = tEngine.getGpuState();
    auto frameCache = gpuState->getFrameCache();

    { // Create a root signature
        GpuRootDescriptor vertexBuffers[]    = {
                { .mRootIndex = 0, .mType = GpuDescriptorType::Srv },
        };

        GpuRootConstant   perDrawConstants[] = {{ .mRootIndex = 1, .mNum32bitValues = sizeof(VertexDrawConstants) / 4 } };
        const char*       debugName          = "Hello Triangle RS";

        GpuRootSignatureInfo Info = {};
        Info.mDescriptors         = farray(vertexBuffers,    ArrayCount(vertexBuffers));
        Info.mDescriptorConstants = farray(perDrawConstants, ArrayCount(perDrawConstants));
        Info.mName                = debugName;
        mRootSignature = GpuRootSignature(*frameCache->getDevice(), Info);
    }

    { // Create a pipeline state object
        ct::ShaderResource vertexShader = tEngine.loadShader("TestTriangle", ct::ShaderStage::Vertex);
        ct::ShaderResource pixelShader  = tEngine.loadShader("TestTriangle", ct::ShaderStage::Pixel);

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
        f32 vertices[] = {
                //   Positions           Color
                -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
                 0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
                 0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
        };

        u16 indices[] = { 0, 1, 2 };

        GpuByteAddressBufferInfo vertexInfo = {
                .mStride = 6 * sizeof(f32),
                .mCount  = 3,
                .mData   = vertices,
        };
        mVertexResource = GpuBuffer::createByteAdressBuffer(frameCache, vertexInfo);

        GpuIndexBufferInfo indexInfo = {
                .mIsU16      = true,
                .mIndexCount = ArrayCount(indices),
                .mIndices    = indices,
        };
        mIndexResource = GpuBuffer::createIndexBuffer(frameCache, indexInfo);

        frameCache->submitCopyCommandList();
        frameCache->flushGpu(); // Forcibly upload all of the geometry (for now)
    }

    return true;
}

bool HelloTriangleApp::onUpdate(ct::Engine& tEngine) {
    return true;
}

bool HelloTriangleApp::onRender(ct::Engine& tEngine) {
    auto gpuState = tEngine.getGpuState();

    gpuState->beginFrame();

    auto frameCache = gpuState->getFrameCache();
    GpuCommandList *commandList = frameCache->getGraphicsCommandList();

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

        f32x4 ClearColor = {0.0f, 0.0f, 0.0f, 1.0f};
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

    if (true) { // Draw the triangle
        commandList->setPipelineState(mPso);
        commandList->setGraphicsRootSignature(mRootSignature);

        commandList->setTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        commandList->setIndexBuffer(mIndexResource.getIndexBufferView());
        commandList->setShaderResourceViewInline(0, mVertexResource.getGpuResource());

        VertexDrawConstants Constants = {};
        commandList->setGraphics32BitConstants(1, &Constants);

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

    gpuState->endFrame();

    return true;
}

bool HelloTriangleApp::onDestroy() {
    mPso.release();
    mRootSignature.release();

    return true;
}

// entry point
#include <Entry.h>