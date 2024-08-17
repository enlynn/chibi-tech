#include "GpuState.h"

#include "d3d12/d3d12.h"
#include "D3D12Common.h"
#include "GpuUtils.h"

constexpr bool cEnableMSAA = false;

GpuState::GpuState(const ct::os::Window& tWindow)
    : mDevice(std::make_shared<GpuDevice>(GpuDeviceInfo{
            .mEnableMSAA = false // todo: make this configurable
        }))
    , mQueueManager(mDevice)
{
    mGlobalResourceState = std::make_unique<GpuGlobalResourceState>();
    mGlobalResourceState->mKnownStates.reserve(10);

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
            .mDevice                    = mDevice,
            .mPresentQueue              = mQueueManager.getQueue(GpuQueueType::Graphics),
            .mSwapchainFormat           = mDevice->getDisplayFormat()
    };

    GpuFrameCache* frameCache = getFrameCache();
    mSwapchain = std::make_unique<GpuSwapchain>(frameCache, swapchainInfo, tWindow);

    // Create the scene framebuffers
    for (auto& cache : mPerFrameCache)
    {
        ForRange(int, FramebufferIndex, int(GpuFramebufferBinding::Max))
        { //todo: remove
            //cache->mFramebuffers[FramebufferIndex] = CreateFramebufferImage(cache.get(), FramebufferIndex == int(GpuFramebufferBinding::DepthStencil));
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
    frameCache->releaseStaleResources();

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
        cache->releaseStaleResources();
    }

    mQueueManager.destroy();
}

void GpuFrameCache::releaseStaleResources() {
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
