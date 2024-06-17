
#include <Game.h>
#include <Engine.h>

#include <Platform/Console.h>

#include <Math/Math.h>
#include <Math/Geometry.h>

#include <Systems/ResourceSystem.h>

#include <Gpu/D3D12Common.h>
#include <Gpu/GpuRenderPass.h>
#include <Gpu/GpuState.h>
#include <Gpu/GpuBuffer.h>

constexpr bool cEnableMSAA = false;

enum class triangle_root_parameter
{
    vertex_buffer = 0,
    mesh_data     = 1,
    per_draw      = 2,
};

struct VertexDrawConstants
{
    u32 uVertexOffset      = 0;
    u32 uVertexBufferIndex = 0;
    u32 uMeshDataIndex     = 0;
};

struct per_mesh_data
{
    mat4 Projection; // Ideally this would go in a "global" constant  buffer. For now, this should be fine.
    mat4 View;
    mat4 Transforms;     // Rotation-Scale-Translation Ops
};

// Example Implementation of a renderpass
class ScenePass
{
public:
    ScenePass() = default;

    void onInit(struct GpuFrameCache* FrameCache);
    void onDeinit(struct GpuFrameCache* FrameCache);
    void onRender(struct GpuFrameCache* FrameCache, class HelloCubeApp* tApp);

private:
    static constexpr const char* cShaderName        = "TestCube";
    static constexpr const char* cRootSignatureName = "Test Cube RS";

    GpuRootSignature mRootSignature{};
    GpuPso           mPso{};
    GpuRenderTarget  mRenderTarget{};
};

class ResolvePass
{
public:
    ResolvePass() = default;

    void onInit(struct GpuFrameCache* FrameCache);
    void onDeinit(struct GpuFrameCache* FrameCache);
    void onRender(struct GpuFrameCache* FrameCache);
};


class HelloCubeApp : public ct::Game {
public:
    [[nodiscard]] ct::GameInfo getGameInfo() const override;

    bool onInit(ct::Engine& tEngine) override;
    bool onUpdate(ct::Engine& tEngine) override;
    bool onRender(ct::Engine& tEngine) override;
    bool onDestroy(ct::Engine& tEngine) override;

    ScenePass            mScenePass{};
    ResolvePass          mResolvePass{};

    GpuBuffer             mVertexResource{};
    GpuBuffer             mIndexResource{};
    GpuBuffer             mPerObjectData{};
};

std::unique_ptr<ct::Game> onGameLoad() {
    return std::make_unique<HelloCubeApp>();
}

ct::GameInfo HelloCubeApp::getGameInfo() const {
    return {
        .mWindowTitle = "Hello Cube",
        .mAssetPath   = HelloCube_CONTENT_PATH,
    };
}

bool HelloCubeApp::onInit(ct::Engine& tEngine) {
    auto gpuState = tEngine.getGpuState();
    auto frameCache = gpuState->getFrameCache();

    mScenePass.onInit(frameCache);
    mResolvePass.onInit(frameCache);

    ct::GeometryCube exampleCube = ct::makeCube(0.5f);

    GpuByteAddressBufferInfo vertexInfo = {
            .mStride = sizeof(exampleCube.mVertices[0]),
            .mCount  = ArrayCount(exampleCube.mVertices),
            .mData   = exampleCube.mVertices,
    };
    mVertexResource = GpuBuffer::createByteAdressBuffer(frameCache, vertexInfo);

    GpuIndexBufferInfo indexInfo = {
            .mIsU16      = true,
            .mIndexCount = ArrayCount(exampleCube.mIndices),
            .mIndices    = exampleCube.mIndices,
    };
    mIndexResource = GpuBuffer::createIndexBuffer(frameCache, indexInfo);

    GpuStructuredBufferInfo PerObjectInfo = {};
    PerObjectInfo.mCount  = 1; // only one object for now
    PerObjectInfo.mStride = sizeof(per_mesh_data);
    PerObjectInfo.mFrames = 3;
    mPerObjectData = GpuBuffer::createStructuredBuffer(frameCache, PerObjectInfo);

    frameCache->submitCopyCommandList();
    frameCache->flushGpu(); // Forcibly upload all of the geometry (for now)

    return true;
}

bool HelloCubeApp::onUpdate(ct::Engine& tEngine) {
    auto gpuState = tEngine.getGpuState();

    { // Let's make the cube spin!
        mPerObjectData.map(gpuState->mFrameCount);
        auto* meshData = (per_mesh_data*) mPerObjectData.getMappedData();

        u32 windowWidth, windowHeight;
        gpuState->mSwapchain->getDimensions(windowWidth, windowHeight);

        mat4 ProjectionMatrix = perspectiveMatrixRh(45.0f, (f32) windowWidth / (f32) windowHeight, 0.01f, 100.0f);
        mat4 LookAtMatrix     = lookAtMatrixRh({0, 0, 5}, {0, 0, -1}, {0, 1, 0});

        meshData[0].Projection = mat4MulRH(ProjectionMatrix, LookAtMatrix);
        meshData[0].View       = {};

        static f32 spinnyTheta = 0.0f;
        spinnyTheta += 0.16f;

        float3 axis = {1, 1, 0};

        quaternion spinnyQuat = {};
        spinnyQuat.XYZ = axis;
        spinnyQuat.Theta = spinnyTheta;

        mat4 spinnyMatrix      = RotateMatrix(spinnyTheta, {1, 1, 1});
        mat4 translationMatrix = translateMatrix({0, 0, -2});

        meshData[0].Transforms = mat4MulRH(translationMatrix, spinnyMatrix);

        mPerObjectData.unmap();
    }

    return true;
}

bool HelloCubeApp::onRender(ct::Engine& tEngine) {
    auto gpuState = tEngine.getGpuState();

    gpuState->beginFrame();

    auto frameCache = gpuState->getFrameCache();
    mScenePass.onRender(frameCache, this);
    mResolvePass.onRender(frameCache);

    gpuState->endFrame();

    return true;
}

bool HelloCubeApp::onDestroy(ct::Engine& tEngine) {
    // TODO:
    return true;
}

void ScenePass::onInit(struct GpuFrameCache *FrameCache)
{
    auto pEngine = ct::getEngine();

    ct::ShaderResource vertexShader = pEngine->loadShader(cShaderName, ct::ShaderStage::Vertex);
    ct::ShaderResource pixelShader  = pEngine->loadShader(cShaderName, ct::ShaderStage::Pixel);

    { // Create a simple root signature
        GpuRootDescriptor VertexBuffers[]    = {
                { .mRootIndex = u32(triangle_root_parameter::vertex_buffer), .mType = GpuDescriptorType::Srv },
                { .mRootIndex = u32(triangle_root_parameter::mesh_data),     .mType = GpuDescriptorType::Srv, .mFlags = GpuDescriptorRangeFlags::None, .mShaderRegister = 1, .mRegisterSpace = 1 }
        };

        GpuRootConstant   PerDrawConstants[] = {{ .mRootIndex = u32(triangle_root_parameter::per_draw), .mNum32bitValues = sizeof(VertexDrawConstants) / 4 } };
        const char*       DebugName          = "Scene Pass Root Signature";

        GpuRootSignatureInfo Info = {};
        Info.mDescriptors         = farray(VertexBuffers, ArrayCount(VertexBuffers));
        Info.mDescriptorConstants = farray(PerDrawConstants, ArrayCount(PerDrawConstants));
        Info.mName                = DebugName;
        mRootSignature = GpuRootSignature(*FrameCache->getDevice(), Info);
    }

    DXGI_FORMAT RTFormat = FrameCache->mGlobal->mSwapchain->getSwapchainFormat();

    { // Create a simple pso
        D3D12_RT_FORMAT_ARRAY PsoRTFormats = {};
        PsoRTFormats.NumRenderTargets = 1;
        PsoRTFormats.RTFormats[0]     = RTFormat;

        u32 SampleCount   = 1;
        u32 SampleQuality = 0;
        if (cEnableMSAA)
        {
            GpuDevice* Device = FrameCache->getDevice();

            DXGI_SAMPLE_DESC Samples = Device->getMultisampleQualityLevels(RTFormat);
            SampleCount   = Samples.Count;
            SampleQuality = Samples.Quality;
        }

        GpuGraphicsPsoBuilder Builder = GpuGraphicsPsoBuilder::builder();
        Builder.setRootSignature(&mRootSignature)
                .setVertexShader(vertexShader)
                .setPixelShader(pixelShader)
                .setRenderTargetFormats({FrameCache->mGlobal->mSwapchain->getSwapchainFormat()})
                .setSampleQuality(SampleCount, SampleQuality)
                .setDepthStencilState(getDepthStencilState(GpuDepthStencilState::ReadWrite), DXGI_FORMAT_D32_FLOAT);

        mPso = Builder.compile(FrameCache);
    }
}

void ScenePass::onDeinit(struct GpuFrameCache *FrameCache)
{
    mPso.release();
    mRootSignature.release();

    mRenderTarget.reset();
}

void ScenePass::onRender(struct GpuFrameCache *FrameCache, struct HelloCubeApp *tApp)
{
    mRenderTarget.reset();

    GpuTexture* SceneFramebuffer = FrameCache->getFramebuffer(GpuFramebufferBinding::MainColor);
    GpuTexture* DepthBuffer      = FrameCache->getFramebuffer(GpuFramebufferBinding::DepthStencil);

    mRenderTarget.attachTexture(AttachmentPoint::Color0, SceneFramebuffer);
    mRenderTarget.attachTexture(AttachmentPoint::DepthStencil, DepthBuffer);

    GpuCommandList* commandList = FrameCache->getGraphicsCommandList();

    // Clear the Framebuffer and bind the render target.
    FrameCache->transitionResource(SceneFramebuffer->getResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    FrameCache->transitionResource(DepthBuffer->getResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

    float4 ClearColor = {0.0f, 0.0f, 0.0f, 1.0f };
    commandList->bindRenderTarget(&mRenderTarget, &ClearColor, true);

    // Scissor / Viewport
    D3D12_VIEWPORT Viewport = mRenderTarget.getViewport();
    commandList->setViewport(Viewport);

    D3D12_RECT ScissorRect = {};
    ScissorRect.left   = 0;
    ScissorRect.top    = 0;
    ScissorRect.right  = (LONG)Viewport.Width;
    ScissorRect.bottom = (LONG)Viewport.Height;
    commandList->setScissorRect(ScissorRect);

    // Draw some geomtry

    commandList->setPipelineState(mPso);
    commandList->setGraphicsRootSignature(mRootSignature);

    commandList->setTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->setIndexBuffer(tApp->mIndexResource.getIndexBufferView());
    commandList->setShaderResourceViewInline(u32(triangle_root_parameter::vertex_buffer),
                                             tApp->mVertexResource.getGpuResource());
    commandList->setShaderResourceViewInline(u32(triangle_root_parameter::mesh_data),
                                             tApp->mPerObjectData.getGpuResource(),
                                             tApp->mPerObjectData.getMappedDataOffset());

    VertexDrawConstants Constants = {};
    commandList->setGraphics32BitConstants(u32(triangle_root_parameter::per_draw), &Constants);

    FrameCache->flushResourceBarriers(commandList); // flush barriers before drawing

    commandList->drawIndexedInstanced(tApp->mIndexResource.getIndexCount());
}

void ResolvePass::onInit(struct GpuFrameCache *FrameCache)   {} // Does not need to initialize frame resources
void ResolvePass::onDeinit(struct GpuFrameCache *FrameCache) {} // No Resources to clean up

void ResolvePass::onRender(struct GpuFrameCache *FrameCache)
{
    GpuCommandList* commandList = FrameCache->getGraphicsCommandList();

    GpuRenderTarget* SwapchainRenderTarget = FrameCache->mGlobal->mSwapchain->getRenderTarget();

    const GpuTexture* SwapchainBackBuffer = SwapchainRenderTarget->getTexture(AttachmentPoint::Color0);
    const GpuTexture* SceneTexture        = FrameCache->getFramebuffer(GpuFramebufferBinding::MainColor);

    bool MSAAEnabled = SceneTexture->getResourceDesc().SampleDesc.Count > 1;
    if (MSAAEnabled)
    {
        commandList->resolveSubresource(FrameCache, SwapchainBackBuffer->getResource(), SceneTexture->getResource());
    }
    else
    {
        commandList->copyResource(FrameCache, SwapchainBackBuffer->getResource(), SceneTexture->getResource());
    }

    FrameCache->transitionResource(SwapchainBackBuffer->getResource(), D3D12_RESOURCE_STATE_PRESENT);
    FrameCache->flushResourceBarriers(commandList);
}

// entry point
#include <Entry.h>