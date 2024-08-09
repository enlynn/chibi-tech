#pragma once

#include "D3D12Common.h"
#include "GpuDevice.h"
#include "GpuResource.h"
#include "GpuCommandList.h"
#include "GpuQueue.h"
#include "GpuQueueManager.h"
#include "GpuSwapchain.h"
#include "GpuBuffer.h"
#include "GpuRootSignature.h"
#include "GpuPso.h"
#include "GpuResourceState.h"

#include <Systems/ResourceSystem.h>

#include <memory>
#include <array>

namespace ct::os {
    class Window;
}

enum class GpuFramebufferBinding
{
    MainColor,
    DepthStencil,
    Max,
};

struct GpuState
{
    explicit GpuState(const ct::os::Window& tWindow);
    void destroy();

    bool beginFrame();
    bool endFrame();

    CpuDescriptor allocateCpuDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE tDescriptorType, u32 tNumDescriptors = 1);
    void releaseDescriptors(CpuDescriptor tDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE tDescriptorType);

	u64                        mFrameCount = 0;

    // For loader Shader Sources - TODO: probably  remove?
    ResourceSystem*                mResourceSystem;

	std::unique_ptr<GpuDevice>     mDevice{nullptr};
    std::unique_ptr<GpuSwapchain>  mSwapchain{nullptr};

    /*std::unique_ptr<GpuQueue> mGraphicsQueue{nullptr};
    std::unique_ptr<GpuQueue> mComputeQueue{nullptr};
    std::unique_ptr<GpuQueue> mCopyQueue{nullptr};*/
    GpuQueueManager                mQueueManager;

	std::array<CpuDescriptorAllocator, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mStaticDescriptors;  // Global descriptors, long lived

    std::unique_ptr<GpuGlobalResourceState> mGlobalResourceState;

	static constexpr  int   cMaxFrameCache = 5; // Keep up to 5 frames in the cache
	std::array<std::unique_ptr<GpuFrameCache>, cMaxFrameCache> mPerFrameCache{};

	[[nodiscard]] inline struct GpuFrameCache* getFrameCache() const;
};

struct GpuFrameCache
{
	GpuState*                  mGlobal = nullptr;
	std::vector<GpuResource>   mStaleResources = {}; // Resources that needs to be freed. Is freed on next use
    std::vector<ID3D12Object*> mStaleObjects{};

	GpuCommandListUPtr         mGraphicsList = nullptr;
	GpuCommandListUPtr         mCopyList     = nullptr;
	GpuCommandListUPtr         mComputeList  = nullptr;

    GpuResourceStateTracker    mResourceStateTracker = {};

    // NOTE(Enlynn): ideally we would create this from a scratch memory texture pool, but for now,
    // let's just create one texture per FrameCache
    // TODO(enlynn): Make this a dictionary<FramebufferIdentifier, std::vector<GpuTextures>>
    std::array<GpuTexture, u32(GpuFramebufferBinding::Max)> mFramebuffers{};

    //
    // Convenient Wrapper functions to make accessing state a bit simpler
    //

    [[nodiscard]] GpuDevice* getDevice() const { return mGlobal->mDevice.get(); }

    GpuTexture* getFramebuffer(GpuFramebufferBinding Binding) { return &mFramebuffers[u32(Binding)]; }

    void flushGpu()
    {
        mGlobal->mQueueManager.flushGpu();
        // TODO(enlynn): flush the mCompute and Copy Queues
    }

	// Convenience Function to get the active command list
	// For now, just using the Graphics command list
	GpuCommandList* borrowGraphicsCommandList() { if (!mGraphicsList) { mGraphicsList = mGlobal->mQueueManager.getGraphicsCommandList(); } return mGraphicsList.get(); }
	GpuCommandList* borrowCopyCommandList()     { if (!mGraphicsList) { mGraphicsList = mGlobal->mQueueManager.getGraphicsCommandList(); } return mGraphicsList.get(); }
	GpuCommandList* borrowComputeCommandList()  { if (!mGraphicsList) { mGraphicsList = mGlobal->mQueueManager.getGraphicsCommandList(); } return mGraphicsList.get(); }

	// Convenience Function to submit the active command list
	// For now, just using the Graphics command list
	void submitGraphicsCommandList() { submitGraphicsCommandList(std::move(mGraphicsList)); mGraphicsList = nullptr; }
	void submitCopyCommandList()     { submitGraphicsCommandList(std::move(mGraphicsList)); mGraphicsList = nullptr; }
	void submitComputeCommandList()  { submitGraphicsCommandList(std::move(mGraphicsList)); mGraphicsList = nullptr; }

	// Similar to the above functions, but can submit a one-time use command list
	void submitGraphicsCommandList(GpuCommandListUPtr tCommandList)
    {
        if (tCommandList)
        {
            // Get any initial barrier transitions and make sure they happen first
            GpuCommandListUPtr pendingBarriersList = mGlobal->mQueueManager.getGraphicsCommandList();
            u32 numPendingBarriers = mGlobal->mGlobalResourceState->flushPendingResourceBarriers(*pendingBarriersList, mResourceStateTracker);

            // Submit the command lists
            if (numPendingBarriers > 0)
            {
                GpuCommandListUPtr lists[2] = { std::move(pendingBarriersList), std::move(tCommandList) };
                mGlobal->mQueueManager.submitCommandLists(std::span(lists, 2));
            }
            else
            {
                // no barriers recorded. return list to waiting queue.
                mGlobal->mQueueManager.submitEmptyCommandList(std::move(pendingBarriersList));
                mGlobal->mQueueManager.submitCommandList(std::move(tCommandList));
            }

            mGlobal->mGlobalResourceState->submitResourceStates(mResourceStateTracker);
        }
    }

    void submitCopyCommandList(GpuCommandListUPtr tCommandList) {
        if (tCommandList) {
            mGlobal->mQueueManager.submitCommandList(std::move(tCommandList));
        }
    }

    void submitComputeCommandList(GpuCommandListUPtr tCommandList)  { 
        if (tCommandList) {
            mGlobal->mQueueManager.submitCommandList(std::move(tCommandList));
        }
    }

    // Add a stale Resource to the queue to be freed eventually
    void addStaleResource(GpuResource Resource)                  { mStaleResources.push_back(Resource); }
    void addStaleObject(ID3D12Object* tObject)                   { mStaleObjects.push_back(tObject);    }

    void trackResource(class GpuResource& Resource, D3D12_RESOURCE_STATES InitialState, UINT SubResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) const
    {
        mGlobal->mGlobalResourceState->addResource(Resource, InitialState, SubResource);
    }

    void removeTrackedResource(const class GpuResource& Resource) const
    {
        mGlobal->mGlobalResourceState->removeResource(Resource);
    }

    // Push a transition Resource barier
    void transitionResource(const GpuResource*    tResource,
                            D3D12_RESOURCE_STATES tStateAfter,
                            UINT                  tFirstSubResource = 0,
                            UINT                  tNumSubResources  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
    {
        if (tNumSubResources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
        {
            for (u32 i = 0; i < tNumSubResources; ++i)
            {
                mResourceStateTracker.transitionBarrier(tResource, tStateAfter, tFirstSubResource + i);
            }
        }
        else
        {
            mResourceStateTracker.transitionBarrier(tResource, tStateAfter);
        }
    }

    // Push a UAV Barrier for a given Resource
    // @param Resource: if null, then any UAV access could require the barrier
    void uavBarrier(const GpuResource* Resource = nullptr) { mResourceStateTracker.uavBarrier(Resource); }

    // Push an aliasing barrier for a given Resource.
    // @param: both can be null, which indicates that any placed/reserved Resource could cause aliasing
    void aliasBarrier(GpuResource* ResourceBefore = nullptr, GpuResource* ResourceAfter = 0)
    {
        mResourceStateTracker.aliasBarrier(ResourceBefore, ResourceAfter);
    }

    void flushResourceBarriers(GpuCommandList* CommandList) { mResourceStateTracker.flushResourceBarriers(CommandList); }

    void releaseStateResources();
};

inline GpuFrameCache* GpuState::getFrameCache() const { return mPerFrameCache[mFrameCount % cMaxFrameCache].get(); }
