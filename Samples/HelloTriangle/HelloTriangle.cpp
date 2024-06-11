
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

struct vertex_draw_constants
{
    u32 uVertexOffset      = 0;
    u32 uVertexBufferIndex = 0;
    u32 uMeshDataIndex     = 0;
};

struct per_mesh_data
{
    f32x44 Projection; // Ideally this would go in a "global" constant  buffer. For now, this should be fine.
    f32x44 View;
    f32x44 Transforms;     // Rotation-Scale-Translation Ops
};

// Example Implementation of a renderpass
class scene_pass
{
public:
    scene_pass() = default;

    void OnInit(struct GpuFrameCache* FrameCache);
    void OnDeinit(struct GpuFrameCache* FrameCache);
    void OnRender(struct GpuFrameCache* FrameCache, class HelloTriangleApp* tApp);

private:
    static constexpr const char* ShaderName        = "TestTriangle";
    static constexpr const char* RootSignatureName = "Test Triangle Name";

    GpuRootSignature mRootSignature{};
    GpuPso            mPso{};
    GpuRenderTarget  mRenderTarget{};
};

class resolve_pass
{
public:
    resolve_pass() = default;

    void OnInit(struct GpuFrameCache* FrameCache);
    void OnDeinit(struct GpuFrameCache* FrameCache);
    void OnRender(struct GpuFrameCache* FrameCache);
};


class HelloTriangleApp : public ct::Game {
public:
    HelloTriangleApp();

    [[nodiscard]] ct::GameInfo getGameInfo() const override;

    bool onInit(ct::Engine& tEngine) override;
    bool onUpdate(ct::Engine& tEngine) override;
    bool onRender(ct::Engine& tEngine) override;
    bool onDestroy() override;

    scene_pass            mScenePass{};
    resolve_pass          mResolvePass{};

    std::filesystem::path mContentDir{};
    ResourceSystem        mResourceSystem{};

    GpuBuffer             mVertexResource{};
    GpuBuffer             mIndexResource{};
    GpuBuffer             mPerObjectData{};
};

std::unique_ptr<ct::Game> onGameLoad() {
    return std::make_unique<HelloTriangleApp>();
}

ct::GameInfo HelloTriangleApp::getGameInfo() const {
    return {
        .mWindowTitle = "Hello Triangle",
        .mAssetPath   = CONTENT_PATH,
    };
}

HelloTriangleApp::HelloTriangleApp()
: mContentDir(std::filesystem::path(CONTENT_PATH))
, mResourceSystem(mContentDir)
{
}

bool HelloTriangleApp::onInit(ct::Engine& tEngine) {
    auto gpuState = tEngine.getGpuState();
    auto frameCache = gpuState->getFrameCache();

    mScenePass.OnInit(frameCache);
    mResolvePass.OnInit(frameCache);

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

bool HelloTriangleApp::onUpdate(ct::Engine& tEngine) {
    auto gpuState = tEngine.getGpuState();

    { // Let's make the cube spin!
        mPerObjectData.map(gpuState->mFrameCount);
        auto* meshData = (per_mesh_data*) mPerObjectData.getMappedData();

        u32 windowWidth, windowHeight;
        gpuState->mSwapchain->getDimensions(windowWidth, windowHeight);

        f32x44 ProjectionMatrix = PerspectiveMatrixRH(45.0f, (f32)windowWidth / (f32)windowHeight, 0.01f, 100.0f);
        f32x44 LookAtMatrix     = LookAtMatrixRH({0,0,5}, {0,0,-1}, {0,1,0});

        meshData[0].Projection = F32x44MulRH(ProjectionMatrix, LookAtMatrix);
        meshData[0].View       = {};

        static f32 spinnyTheta = 0.0f;
        spinnyTheta += 0.16f;

        f32x3 axis = {1, 1, 0};

        quaternion spinnyQuat = {};
        spinnyQuat.XYZ = axis;
        spinnyQuat.Theta = spinnyTheta;

        f32x44 spinnyMatrix      = RotateMatrix(spinnyTheta, {1,1,1});
        f32x44 translationMatrix = TranslateMatrix({ 0, 0, -2});

        meshData[0].Transforms = F32x44MulRH(translationMatrix, spinnyMatrix);

        mPerObjectData.unmap();
    }

    return true;
}

bool HelloTriangleApp::onRender(ct::Engine& tEngine) {
    auto gpuState = tEngine.getGpuState();

    gpuState->beginFrame();

    auto frameCache = gpuState->getFrameCache();
    mScenePass.OnRender(frameCache, this);
    mResolvePass.OnRender(frameCache);

    gpuState->endFrame();

    return true;
}

bool HelloTriangleApp::onDestroy() {
    // TODO:
    return true;
}

void scene_pass::OnInit(GpuFrameCache* tFrameCache)
{
    auto pEngine = ct::getEngine();

    ct::ShaderResource vertexShader = pEngine->loadShader("TestTriangle", ct::ShaderStage::Vertex);
    ct::ShaderResource pixelShader  = pEngine->loadShader("TestTriangle", ct::ShaderStage::Pixel);

    { // Create a simple root signature
        GpuRootDescriptor VertexBuffers[]    = {
                { .mRootIndex = u32(triangle_root_parameter::vertex_buffer), .mType = GpuDescriptorType::Srv },
                { .mRootIndex = u32(triangle_root_parameter::mesh_data),     .mType = GpuDescriptorType::Srv, .mFlags = GpuDescriptorRangeFlags::None, .mShaderRegister = 1, .mRegisterSpace = 1 }
        };

        GpuRootConstant   PerDrawConstants[] = {{ .mRootIndex = u32(triangle_root_parameter::per_draw), .mNum32bitValues = sizeof(vertex_draw_constants) / 4 } };
        const char*         DebugName          = "Scene Pass Root Signature";

        GpuRootSignatureInfo Info = {};
        Info.mDescriptors         = farray(VertexBuffers, ArrayCount(VertexBuffers));
        Info.mDescriptorConstants = farray(PerDrawConstants, ArrayCount(PerDrawConstants));
        Info.mName                = DebugName;
        mRootSignature = GpuRootSignature(*tFrameCache->getDevice(), Info);
    }

    DXGI_FORMAT RTFormat = tFrameCache->mGlobal->mSwapchain->getSwapchainFormat();

    { // Create a simple pso
        D3D12_RT_FORMAT_ARRAY PsoRTFormats = {};
        PsoRTFormats.NumRenderTargets = 1;
        PsoRTFormats.RTFormats[0]     = RTFormat;

        u32 SampleCount   = 1;
        u32 SampleQuality = 0;
        if (cEnableMSAA)
        {
            GpuDevice* Device = tFrameCache->getDevice();

            DXGI_SAMPLE_DESC Samples = Device->getMultisampleQualityLevels(RTFormat);
            SampleCount   = Samples.Count;
            SampleQuality = Samples.Quality;
        }

        GpuGraphicsPsoBuilder Builder = GpuGraphicsPsoBuilder::builder();
        Builder.setRootSignature(&mRootSignature)
                .setVertexShader(vertexShader)
                .setPixelShader(pixelShader)
                .setRenderTargetFormats({tFrameCache->mGlobal->mSwapchain->getSwapchainFormat()})
                .setSampleQuality(SampleCount, SampleQuality)
                .setDepthStencilState(getDepthStencilState(GpuDepthStencilState::ReadWrite), DXGI_FORMAT_D32_FLOAT);

        mPso = Builder.compile(tFrameCache);
    }
}

void scene_pass::OnDeinit(GpuFrameCache* FrameCache)
{
    mPso.release();
    mRootSignature.release();

    mRenderTarget.reset();
}

void scene_pass::OnRender(GpuFrameCache* tFrameCache, HelloTriangleApp* tApp)
{
    mRenderTarget.reset();

    GpuTexture* SceneFramebuffer = tFrameCache->getFramebuffer(GpuFramebufferBinding::MainColor);
    GpuTexture* DepthBuffer      = tFrameCache->getFramebuffer(GpuFramebufferBinding::DepthStencil);

    mRenderTarget.attachTexture(AttachmentPoint::Color0, SceneFramebuffer);
    mRenderTarget.attachTexture(AttachmentPoint::DepthStencil, DepthBuffer);

    GpuCommandList* commandList = tFrameCache->getGraphicsCommandList();

    // Clear the Framebuffer and bind the render target.
    tFrameCache->transitionResource(SceneFramebuffer->getResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    tFrameCache->transitionResource(DepthBuffer->getResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

    f32x4 ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
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

    vertex_draw_constants Constants = {};
    commandList->setGraphics32BitConstants(u32(triangle_root_parameter::per_draw), &Constants);

    tFrameCache->flushResourceBarriers(commandList); // flush barriers before drawing

    commandList->drawIndexedInstanced(tApp->mIndexResource.getIndexCount());
}

void resolve_pass::OnInit(GpuFrameCache* FrameCache)   {} // Does not need to initialize frame resources
void resolve_pass::OnDeinit(GpuFrameCache* FrameCache) {} // No Resources to clean up

void resolve_pass::OnRender(GpuFrameCache* tFrameCache)
{
    GpuCommandList* commandList = tFrameCache->getGraphicsCommandList();

    GpuRenderTarget* SwapchainRenderTarget = tFrameCache->mGlobal->mSwapchain->getRenderTarget();

    const GpuTexture* SwapchainBackBuffer = SwapchainRenderTarget->getTexture(AttachmentPoint::Color0);
    const GpuTexture* SceneTexture        = tFrameCache->getFramebuffer(GpuFramebufferBinding::MainColor);

    bool MSAAEnabled = SceneTexture->getResourceDesc().SampleDesc.Count > 1;
    if (MSAAEnabled)
    {
        commandList->resolveSubresource(tFrameCache, SwapchainBackBuffer->getResource(), SceneTexture->getResource());
    }
    else
    {
        commandList->copyResource(tFrameCache, SwapchainBackBuffer->getResource(), SceneTexture->getResource());
    }

    tFrameCache->transitionResource(SwapchainBackBuffer->getResource(), D3D12_RESOURCE_STATE_PRESENT);
    tFrameCache->flushResourceBarriers(commandList);
}

// entry point
#include <Entry.h>