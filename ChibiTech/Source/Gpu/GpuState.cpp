#include "GpuState.h"

#include "d3d12/d3d12.h"
#include "D3D12Common.h"
#include "gpu_utils.h"

constexpr bool cEnableMSAA = false;

namespace {
    gpu_texture CreateFramebufferImage(gpu_frame_cache* tFrameCache, bool tIsDepth = false)
    {
        gpu_swapchain* swapchain = tFrameCache->mGlobal->mSwapchain.get();

        // In the future, will want to make this variable.
        DXGI_FORMAT FramebufferFormat = swapchain->GetSwapchainFormat();

        // For now, these will match the dimensions of the swapchain
        u32 FramebufferWidth, FramebufferHeight;
        swapchain->GetDimensions(FramebufferWidth, FramebufferHeight);

        u32 SampleCount   = 1;
        u32 SampleQuality = 0;
        if (cEnableMSAA)
        {
            gpu_device* Device = tFrameCache->GetDevice();

            DXGI_SAMPLE_DESC Samples = Device->GetMultisampleQualityLevels(FramebufferFormat);
            SampleCount   = Samples.Count;
            SampleQuality = Samples.Quality;
        }

        // If this is
        FramebufferFormat = (tIsDepth) ? DXGI_FORMAT_D32_FLOAT : FramebufferFormat;

        D3D12_RESOURCE_DESC ImageDesc = GetTex2DDesc(
                FramebufferFormat, FramebufferWidth, FramebufferHeight,
                1, 1, SampleCount, SampleQuality);

        // Allow UAV in case a Renderpass wants to read from the Image
        if (tIsDepth)
            ImageDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        else
            ImageDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET|D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        D3D12_CLEAR_VALUE ClearValue = {};
        ClearValue.Format   = ImageDesc.Format;

        if (tIsDepth)
        {
            ClearValue.DepthStencil = {1.0f, 0 };
        }
        else
        {
            ClearValue.Color[0] = 0.0f;
            ClearValue.Color[1] = 0.0f;
            ClearValue.Color[2] = 0.0f;
            ClearValue.Color[3] = 1.0f;
        }

        gpu_resource ImageResource = gpu_resource(*tFrameCache->GetDevice(), ImageDesc, ClearValue);
        return gpu_texture(tFrameCache, ImageResource);
    }
}

GpuState::GpuState(const ct::os::Window& tWindow) {

    mDevice = std::make_unique<gpu_device>();
    mDevice->Init();

    mGraphicsQueue = std::make_unique<gpu_command_queue>(gpu_command_queue_type::graphics, mDevice.get());
    mCopyQueue     = std::make_unique<gpu_command_queue>(gpu_command_queue_type::copy,     mDevice.get());
    mComputeQueue  = std::make_unique<gpu_command_queue>(gpu_command_queue_type::compute,  mDevice.get());

    mGlobalResourceState = std::make_unique<gpu_global_resource_state>();
    mGlobalResourceState->mKnownStates.reserve(10);

    for (size_t i = 0; i < mStaticDescriptors.size(); ++i) {
        mStaticDescriptors[i].Init(mDevice.get(), (D3D12_DESCRIPTOR_HEAP_TYPE)i);
    }

    //
    // Let's setup the the Per Frame Resources and let Frame 0 be the setup frame
    //

    for (auto& cache : mPerFrameCache)
    {
        cache = std::make_unique<gpu_frame_cache>();
        cache->mGlobal               = this;
        cache->mResourceStateTracker = gpu_resource_state_tracker();
        cache->mStaleResources.reserve(5);
    }

    //
    // Let's setup the swapchain now
    //

    gpu_swapchain_info swapchainInfo = {
            .mDevice                    = mDevice.get(),
            .mPresentQueue              = mGraphicsQueue.get(),
            .mRenderTargetDesciptorHeap = &mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_RTV],
            // Leave everything else at their defaults for now...
    };

    gpu_frame_cache* frameCache = GetFrameCache();
    mSwapchain = std::make_unique<gpu_swapchain>(frameCache, swapchainInfo, tWindow);

    // Create the scene framebuffers
    for (auto& cache : mPerFrameCache)
    {
        ForRange(int, FramebufferIndex, int(gpu_framebuffer_binding::max))
        {
            cache->mFramebuffers[FramebufferIndex] = CreateFramebufferImage(cache.get(), FramebufferIndex == int(gpu_framebuffer_binding::depth_stencil));
        }
    }

}

bool GpuState::beginFrame() {
    gpu_frame_cache* frameCache = GetFrameCache();

    //------------------------------------------------------------------------------------------------------------------
    // Begin Frame Cache Cleanup

    // Reset the Per-Frame State Tracker
    frameCache->mResourceStateTracker.Reset();

    // Update any pending command lists
    mGraphicsQueue->ProcessCommandLists();
    mCopyQueue->ProcessCommandLists();
    mComputeQueue->ProcessCommandLists();

    // Clean up any previous resources
    frameCache->releaseStateResources();

    // End Frame Cache Cleanup
    //------------------------------------------------------------------------------------------------------------------
    // Begin Data Setup

    return true;
}

bool GpuState::endFrame() {
    gpu_frame_cache* frameCache = GetFrameCache();

    frameCache->SubmitGraphicsCommandList();
    frameCache->SubmitComputeCommandList();
    frameCache->SubmitCopyCommandList();

    mSwapchain->Present();

    mFrameCount += 1;

    return true;
}

void GpuState::destroy() {
    mGraphicsQueue->Flush();
    mCopyQueue->Flush();
    mComputeQueue->Flush();

    gpu_frame_cache* frameCache = GetFrameCache();

    mSwapchain->Release(frameCache);

    // Free any stale resources
    for (auto& cache : mPerFrameCache)
    {
        cache->releaseStateResources();
    }
}

void gpu_frame_cache::releaseStateResources() {
    for (gpu_resource& Resource : mStaleResources)
    {
        ID3D12Resource* ResourceHandle = Resource.AsHandle();
        ComSafeRelease(ResourceHandle);
    }

    mStaleResources.clear();
}
