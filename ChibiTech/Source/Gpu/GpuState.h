#pragma once

#include "D3D12Common.h"
#include "GpuDevice.h"
#include "GpuResource.h"
#include "GpuCommandList.h"
#include "GpuCommandQueue.h"
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

	u64                        mFrameCount = 0;

    // For loader Shader Sources - TODO: probably  remove?
    ResourceSystem*                mResourceSystem;

	std::unique_ptr<GpuDevice>     mDevice{nullptr};
    std::unique_ptr<GpuSwapchain>  mSwapchain{nullptr};

    std::unique_ptr<GpuCommandQueue> mGraphicsQueue{nullptr};
    std::unique_ptr<GpuCommandQueue> mComputeQueue{nullptr};
    std::unique_ptr<GpuCommandQueue> mCopyQueue{nullptr};

	std::array<CpuDescriptorAllocator, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mStaticDescriptors;  // Global descriptors, long lived

    std::unique_ptr<GpuGlobalResourceState> mGlobalResourceState;

	static constexpr  int   cMaxFrameCache = 5; // Keep up to 5 frames in the cache
	std::array<std::unique_ptr<GpuFrameCache>, cMaxFrameCache> mPerFrameCache{};

	[[nodiscard]] inline struct GpuFrameCache* getFrameCache() const;
};

struct GpuFrameCache
{
	GpuState*                mGlobal = nullptr;
	std::vector<GpuResource> mStaleResources = {}; // Resources that needs to be freed. Is freed on next use

	GpuCommandList*          mGraphicsList = nullptr;
	GpuCommandList*          mCopyList     = nullptr;
	GpuCommandList*          mComputeList  = nullptr;

    GpuResourceStateTracker mResourceStateTracker = {};

    // NOTE(Enlynn): ideally we would create this from a scratch memory texture pool, but for now,
    // let's just create one texture per FrameCache
    std::array<GpuTexture, u32(GpuFramebufferBinding::Max)> mFramebuffers{};

    //
    // Convenient Wrapper functions to make accessing state a bit simpler
    //

    [[nodiscard]] GpuDevice* getDevice() const { return mGlobal->mDevice.get(); }

    GpuTexture* getFramebuffer(GpuFramebufferBinding Binding) { return &mFramebuffers[u32(Binding)]; }

    void flushGpu()
    {
        mGlobal->mGraphicsQueue->flush();
        // TODO(enlynn): flush the mCompute and Copy Queues
    }

	// Convenience Function to get the active command list
	// For now, just using the Graphics command list
	GpuCommandList* getGraphicsCommandList() { if (!mGraphicsList) { mGraphicsList = mGlobal->mGraphicsQueue->getCommandList(); } return mGraphicsList; }
	GpuCommandList* getCopyCommandList()     { if (!mGraphicsList) { mGraphicsList = mGlobal->mGraphicsQueue->getCommandList(); } return mGraphicsList; }
	GpuCommandList* getComputeCommandList()  { if (!mGraphicsList) { mGraphicsList = mGlobal->mGraphicsQueue->getCommandList(); } return mGraphicsList; }

	// Convenience Function to submit the active command list
	// For now, just using the Graphics command list
	void submitGraphicsCommandList() { submitGraphicsCommandList(mGraphicsList); mGraphicsList = nullptr; }
	void submitCopyCommandList()     { submitGraphicsCommandList(mGraphicsList); mGraphicsList = nullptr; }
	void submitComputeCommandList()  { submitGraphicsCommandList(mGraphicsList); mGraphicsList = nullptr; }

	// Similar to the above functions, but can submit a one-time use command list
	void submitGraphicsCommandList(GpuCommandList* CommandList)
    {
        if (CommandList)
        {
            // Get any initial barrier transitions and make sure they happen first
            GpuCommandList* PendingBarriersList = mGlobal->mGraphicsQueue->getCommandList();
            u32 NumPendingBarriers = mGlobal->mGlobalResourceState->flushPendingResourceBarriers(*PendingBarriersList,
                                                                                                 mResourceStateTracker);

            // Submit the command lists
            if (NumPendingBarriers > 0)
            {
                GpuCommandList* ToSubmit[] = {PendingBarriersList, CommandList};
                mGlobal->mGraphicsQueue->executeCommandLists(farray(ToSubmit, 2));
            }
            else
            {
                mGlobal->mGraphicsQueue->submitEmptyCommandList(PendingBarriersList); // no barriers recorded.
                mGlobal->mGraphicsQueue->executeCommandLists(farray(&CommandList, 1));
            }

            mGlobal->mGlobalResourceState->submitResourceStates(mResourceStateTracker);
        }
    }

    void submitCopyCommandList(GpuCommandList* CommandList)     { if (CommandList) {
            mGlobal->mGraphicsQueue->executeCommandLists(farray(&CommandList, 1)); } }
    void submitComputeCommandList(GpuCommandList* CommandList)  { if (CommandList) {
            mGlobal->mGraphicsQueue->executeCommandLists(farray(&CommandList, 1)); } }

    // Add a stale Resource to the queue to be freed eventually
    void addStaleResource(GpuResource Resource)                  { mStaleResources.push_back(Resource); }

    void trackResource(class GpuResource& Resource, D3D12_RESOURCE_STATES InitialState, UINT SubResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) const
    {
        mGlobal->mGlobalResourceState->addResource(Resource, InitialState, SubResource);
    }

    void removeTrackedResource(const class GpuResource& Resource) const
    {
        mGlobal->mGlobalResourceState->removeResource(Resource);
    }

    // Push a transition Resource barier
    void transitionResource(const GpuResource*   Resource,
                            D3D12_RESOURCE_STATES StateAfter,
                            UINT                  SubResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
    {
        mResourceStateTracker.transitionBarrier(Resource, StateAfter, SubResource);
    }

    // Push a UAV Barrier for a given Resource
    // @param Resource: if null, then any UAV access could require the barrier
    void uavBarrier(GpuResource* Resource = 0) { mResourceStateTracker.uavBarrier(Resource); }

    // Push an aliasing barrier for a given Resource.
    // @param: both can be null, which indicates that any placed/reserved Resource could cause aliasing
    void aliasBarrier(GpuResource* ResourceBefore = 0, GpuResource* ResourceAfter = 0)
    {
        mResourceStateTracker.aliasBarrier(ResourceBefore, ResourceAfter);
    }

    void flushResourceBarriers(GpuCommandList* CommandList) { mResourceStateTracker.flushResourceBarriers(CommandList); }

    void releaseStateResources();
};

inline GpuFrameCache* GpuState::getFrameCache() const { return mPerFrameCache[mFrameCount % cMaxFrameCache].get(); }
