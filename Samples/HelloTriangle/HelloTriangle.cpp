
#include <Game.h>
#include <Engine.h>

#include <Platform/Console.h>

#include <Math/Math.h>
#include <Math/Geometry.h>

#include <Systems/ResourceSystem.h>

#include <Gpu/D3D12Common.h>
#include <Gpu/gpu_render_pass.h>
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

    void OnInit(struct gpu_frame_cache* FrameCache);
    void OnDeinit(struct gpu_frame_cache* FrameCache);
    void OnRender(struct gpu_frame_cache* FrameCache, class HelloTriangleApp* tApp);

private:
    static constexpr const char* ShaderName        = "TestTriangle";
    static constexpr const char* RootSignatureName = "Test Triangle Name";

    gpu_root_signature mRootSignature{};
    gpu_pso            mPso{};
    gpu_render_target  mRenderTarget{};
};

class resolve_pass
{
public:
    resolve_pass() = default;

    void OnInit(struct gpu_frame_cache* FrameCache);
    void OnDeinit(struct gpu_frame_cache* FrameCache);
    void OnRender(struct gpu_frame_cache* FrameCache);
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
    auto frameCache = gpuState->GetFrameCache();

    mScenePass.OnInit(frameCache);
    mResolvePass.OnInit(frameCache);

    ct::GeometryCube exampleCube = ct::makeCube(0.5f);

    gpu_byte_address_buffer_info vertexInfo = {
            .mStride = sizeof(exampleCube.mVertices[0]),
            .mCount  = ArrayCount(exampleCube.mVertices),
            .mData   = exampleCube.mVertices,
    };
    mVertexResource = GpuBuffer::CreateByteAdressBuffer(frameCache, vertexInfo);

    gpu_index_buffer_info indexInfo = {
            .mIsU16      = true,
            .mIndexCount = ArrayCount(exampleCube.mIndices),
            .mIndices    = exampleCube.mIndices,
    };
    mIndexResource = GpuBuffer::CreateIndexBuffer(frameCache, indexInfo);

    gpu_structured_buffer_info PerObjectInfo = {};
    PerObjectInfo.mCount  = 1; // only one object for now
    PerObjectInfo.mStride = sizeof(per_mesh_data);
    PerObjectInfo.mFrames = 3;
    mPerObjectData = GpuBuffer::CreateStructuredBuffer(frameCache, PerObjectInfo);

    frameCache->SubmitCopyCommandList();
    frameCache->FlushGPU(); // Forcibly upload all of the geometry (for now)

    return true;
}

bool HelloTriangleApp::onUpdate(ct::Engine& tEngine) {
    auto gpuState = tEngine.getGpuState();

    { // Let's make the cube spin!
        mPerObjectData.Map(gpuState->mFrameCount);
        auto* meshData = (per_mesh_data*)mPerObjectData.GetMappedData();

        u32 windowWidth, windowHeight;
        gpuState->mSwapchain->GetDimensions(windowWidth, windowHeight);

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

        mPerObjectData.Unmap();
    }

    return true;
}

bool HelloTriangleApp::onRender(ct::Engine& tEngine) {
    auto gpuState = tEngine.getGpuState();

    gpuState->beginFrame();

    auto frameCache = gpuState->GetFrameCache();
    mScenePass.OnRender(frameCache, this);
    mResolvePass.OnRender(frameCache);

    gpuState->endFrame();

    return true;
}

bool HelloTriangleApp::onDestroy() {
    // TODO:
    return true;
}

void scene_pass::OnInit(gpu_frame_cache* tFrameCache)
{
    auto pEngine = ct::getEngine();

    ct::ShaderResource vertexShader = pEngine->loadShader("TestTriangle", ct::ShaderStage::Vertex);
    ct::ShaderResource pixelShader  = pEngine->loadShader("TestTriangle", ct::ShaderStage::Pixel);

    { // Create a simple root signature
        gpu_root_descriptor VertexBuffers[]    = {
                { .mRootIndex = u32(triangle_root_parameter::vertex_buffer), .mType = gpu_descriptor_type::srv },
                { .mRootIndex = u32(triangle_root_parameter::mesh_data),     .mType = gpu_descriptor_type::srv, .mFlags = gpu_descriptor_range_flags::none, .mShaderRegister = 1, .mRegisterSpace = 1 }
        };

        gpu_root_constant   PerDrawConstants[] = { { .mRootIndex = u32(triangle_root_parameter::per_draw), .mNum32bitValues = sizeof(vertex_draw_constants) / 4 } };
        const char*         DebugName          = "Scene Pass Root Signature";

        gpu_root_signature_info Info = {};
        Info.Descriptors         = farray(VertexBuffers, ArrayCount(VertexBuffers));
        Info.DescriptorConstants = farray(PerDrawConstants, ArrayCount(PerDrawConstants));
        Info.Name                = DebugName;
        mRootSignature = gpu_root_signature(*tFrameCache->GetDevice(), Info);
    }

    DXGI_FORMAT RTFormat = tFrameCache->mGlobal->mSwapchain->GetSwapchainFormat();

    { // Create a simple pso
        D3D12_RT_FORMAT_ARRAY PsoRTFormats = {};
        PsoRTFormats.NumRenderTargets = 1;
        PsoRTFormats.RTFormats[0]     = RTFormat;

        u32 SampleCount   = 1;
        u32 SampleQuality = 0;
        if (cEnableMSAA)
        {
            gpu_device* Device = tFrameCache->GetDevice();

            DXGI_SAMPLE_DESC Samples = Device->GetMultisampleQualityLevels(RTFormat);
            SampleCount   = Samples.Count;
            SampleQuality = Samples.Quality;
        }

        gpu_graphics_pso_builder Builder = gpu_graphics_pso_builder::Builder();
        Builder.SetRootSignature(&mRootSignature)
                .SetVertexShader(vertexShader)
                .SetPixelShader(pixelShader)
                .SetRenderTargetFormats({ tFrameCache->mGlobal->mSwapchain->GetSwapchainFormat() })
                .SetSampleQuality(SampleCount, SampleQuality)
                .SetDepthStencilState(GetDepthStencilState(gpu_depth_stencil_state::read_write), DXGI_FORMAT_D32_FLOAT);

        mPso = Builder.Compile(tFrameCache);
    }
}

void scene_pass::OnDeinit(gpu_frame_cache* FrameCache)
{
    mPso.Release();
    mRootSignature.Release();

    mRenderTarget.Reset();
}

void scene_pass::OnRender(gpu_frame_cache* tFrameCache, HelloTriangleApp* tApp)
{
    mRenderTarget.Reset();

    gpu_texture* SceneFramebuffer = tFrameCache->GetFramebuffer(gpu_framebuffer_binding::main_color);
    gpu_texture* DepthBuffer      = tFrameCache->GetFramebuffer(gpu_framebuffer_binding::depth_stencil);

    mRenderTarget.AttachTexture(attachment_point::color0, SceneFramebuffer);
    mRenderTarget.AttachTexture(attachment_point::depth_stencil, DepthBuffer);

    gpu_command_list* commandList = tFrameCache->GetGraphicsCommandList();

    // Clear the Framebuffer and bind the render target.
    tFrameCache->TransitionResource(SceneFramebuffer->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    tFrameCache->TransitionResource(DepthBuffer->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

    f32x4 ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    commandList->BindRenderTarget(&mRenderTarget, &ClearColor, true);

    // Scissor / Viewport
    D3D12_VIEWPORT Viewport = mRenderTarget.GetViewport();
    commandList->SetViewport(Viewport);

    D3D12_RECT ScissorRect = {};
    ScissorRect.left   = 0;
    ScissorRect.top    = 0;
    ScissorRect.right  = (LONG)Viewport.Width;
    ScissorRect.bottom = (LONG)Viewport.Height;
    commandList->SetScissorRect(ScissorRect);

    // Draw some geomtry

    commandList->SetPipelineState(mPso);
    commandList->SetGraphicsRootSignature(mRootSignature);

    commandList->SetTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->SetIndexBuffer(tApp->mIndexResource.GetIndexBufferView());
    commandList->SetShaderResourceViewInline(u32(triangle_root_parameter::vertex_buffer), tApp->mVertexResource.GetGPUResource());
    commandList->SetShaderResourceViewInline(u32(triangle_root_parameter::mesh_data), tApp->mPerObjectData.GetGPUResource(), tApp->mPerObjectData.GetMappedDataOffset());

    vertex_draw_constants Constants = {};
    commandList->SetGraphics32BitConstants(u32(triangle_root_parameter::per_draw), &Constants);

    tFrameCache->FlushResourceBarriers(commandList); // Flush barriers before drawing

    commandList->DrawIndexedInstanced(tApp->mIndexResource.GetIndexCount());
}

void resolve_pass::OnInit(gpu_frame_cache* FrameCache)   {} // Does not need to initialize frame resources
void resolve_pass::OnDeinit(gpu_frame_cache* FrameCache) {} // No Resources to clean up

void resolve_pass::OnRender(gpu_frame_cache* tFrameCache)
{
    gpu_command_list* commandList = tFrameCache->GetGraphicsCommandList();

    gpu_render_target* SwapchainRenderTarget = tFrameCache->mGlobal->mSwapchain->GetRenderTarget();

    const gpu_texture* SwapchainBackBuffer = SwapchainRenderTarget->GetTexture(attachment_point::color0);
    const gpu_texture* SceneTexture        = tFrameCache->GetFramebuffer(gpu_framebuffer_binding::main_color);

    bool MSAAEnabled = SceneTexture->GetResourceDesc().SampleDesc.Count > 1;
    if (MSAAEnabled)
    {
        commandList->ResolveSubresource(tFrameCache, SwapchainBackBuffer->GetResource(), SceneTexture->GetResource());
    }
    else
    {
        commandList->CopyResource(tFrameCache, SwapchainBackBuffer->GetResource(), SceneTexture->GetResource());
    }

    tFrameCache->TransitionResource(SwapchainBackBuffer->GetResource(), D3D12_RESOURCE_STATE_PRESENT);
    tFrameCache->FlushResourceBarriers(commandList);
}

// entry point
#include <Entry.h>