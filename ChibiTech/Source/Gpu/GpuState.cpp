#include "GpuState.h"

#include "d3d12/d3d12.h"
#include "D3D12Common.h"
#include "GpuUtils.h"

constexpr bool cEnableMSAA = false;

namespace {
    GpuTexture CreateFramebufferImage(GpuFrameCache* tFrameCache, bool tIsDepth = false)
    {
        GpuSwapchain* swapchain = tFrameCache->mGlobal->mSwapchain.get();

        // In the future, will want to make this variable.
        DXGI_FORMAT FramebufferFormat = swapchain->getSwapchainFormat();

        // For now, these will match the dimensions of the swapchain
        u32 FramebufferWidth, FramebufferHeight;
        swapchain->getDimensions(FramebufferWidth, FramebufferHeight);

        u32 SampleCount   = 1;
        u32 SampleQuality = 0;
        if (cEnableMSAA)
        {
            GpuDevice* Device = tFrameCache->getDevice();

            DXGI_SAMPLE_DESC Samples = Device->getMultisampleQualityLevels(FramebufferFormat);
            SampleCount   = Samples.Count;
            SampleQuality = Samples.Quality;
        }

        // If this is
        FramebufferFormat = (tIsDepth) ? DXGI_FORMAT_D32_FLOAT : FramebufferFormat;

        D3D12_RESOURCE_DESC ImageDesc = getTex2DDesc(
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

        GpuResource ImageResource = GpuResource(*tFrameCache->getDevice(), ImageDesc, ClearValue);
        return GpuTexture(tFrameCache, ImageResource);
    }
}

GpuState::GpuState(const ct::os::Window& tWindow)
    : mDevice(std::make_unique<GpuDevice>(GpuDeviceInfo{
            .mEnableMSAA = false // todo: make this configurable
        }))
    , mQueueManager(mDevice.get())
{
    mGlobalResourceState = std::make_unique<GpuGlobalResourceState>();
    mGlobalResourceState->mKnownStates.reserve(10);

    for (size_t i = 0; i < mStaticDescriptors.size(); ++i) {
        mStaticDescriptors[i].init(mDevice.get(), (D3D12_DESCRIPTOR_HEAP_TYPE) i);
    }

    //
    // Let's setup the the Per Frame Resources and let Frame 0 be the setup frame
    //

    for (auto& cache : mPerFrameCache)
    {
        cache = std::make_unique<GpuFrameCache>();
        cache->mGlobal               = this;
        cache->mResourceStateTracker = GpuResourceStateTracker();
        cache->mStaleResources.reserve(5);
    }

    //
    // Let's setup the swapchain now
    //

    GpuSwapchainInfo swapchainInfo = {
            .mDevice                    = mDevice.get(),
            .mPresentQueue              = mQueueManager.getQueue(GpuQueueType::Graphics),
            .mRenderTargetDesciptorHeap = &mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_RTV],
            // Leave everything else at their defaults for now...
    };

    GpuFrameCache* frameCache = getFrameCache();
    mSwapchain = std::make_unique<GpuSwapchain>(frameCache, swapchainInfo, tWindow);

    // Create the scene framebuffers
    for (auto& cache : mPerFrameCache)
    {
        ForRange(int, FramebufferIndex, int(GpuFramebufferBinding::Max))
        {
            cache->mFramebuffers[FramebufferIndex] = CreateFramebufferImage(cache.get(), FramebufferIndex == int(GpuFramebufferBinding::DepthStencil));
        }
    }

}

bool GpuState::beginFrame() {
    GpuFrameCache* frameCache = getFrameCache();

    //------------------------------------------------------------------------------------------------------------------
    // Begin Frame Cache Cleanup

    // reset the Per-Frame State Tracker
    frameCache->mResourceStateTracker.reset();

    // Update any pending command lists
    mQueueManager.processPendingCommandLists();


    // Clean up any previous resources
    frameCache->releaseStateResources();

    // End Frame Cache Cleanup
    //------------------------------------------------------------------------------------------------------------------
    // Begin Data Setup

    return true;
}

bool GpuState::endFrame() {
    GpuFrameCache* frameCache = getFrameCache();

    frameCache->submitGraphicsCommandList();
    frameCache->submitComputeCommandList();
    frameCache->submitCopyCommandList();

    mSwapchain->present();

    mFrameCount += 1;

    return true;
}

void GpuState::destroy() {
    mQueueManager.flushGpu();

    GpuFrameCache* frameCache = getFrameCache();

    mSwapchain->release(frameCache);

    // Free any stale resources
    for (auto& cache : mPerFrameCache)
    {
        cache->releaseStateResources();
    }
}

CpuDescriptor GpuState::allocateCpuDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE tDescriptorType, u32 tNumDescriptors) {
    return mStaticDescriptors[tDescriptorType].allocate(tNumDescriptors);
}

void GpuState::releaseDescriptors(CpuDescriptor tDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE tDescriptorType) {
    mStaticDescriptors[tDescriptorType].releaseDescriptors(tDescriptor);
}

void GpuFrameCache::releaseStateResources() {
    for (GpuResource& Resource : mStaleResources)
    {
        ID3D12Resource* ResourceHandle = Resource.asHandle();
        ComSafeRelease(ResourceHandle);
    }

    for (auto& object : mStaleObjects) {
        ComSafeRelease(object);
    }

    mStaleResources.clear();
}
