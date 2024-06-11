#pragma once

#include "D3D12Common.h"
#include "gpu_device.h"
#include "gpu_resource.h"
#include "gpu_command_list.h"
#include "gpu_command_queue.h"
#include "gpu_swapchain.h"
#include "GpuBuffer.h"
#include "gpu_root_signature.h"
#include "gpu_pso.h"
#include "gpu_resource_state.h"

#include <Systems/ResourceSystem.h>

#include <memory>
#include <array>

namespace ct::os {
    class Window;
}

enum class gpu_framebuffer_binding
{
    main_color,
    depth_stencil,
    max,
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

	std::unique_ptr<gpu_device>     mDevice{nullptr};
    std::unique_ptr<gpu_swapchain>  mSwapchain{nullptr};

    std::unique_ptr<gpu_command_queue> mGraphicsQueue{nullptr};
    std::unique_ptr<gpu_command_queue> mComputeQueue{nullptr};
    std::unique_ptr<gpu_command_queue> mCopyQueue{nullptr};

	std::array<cpu_descriptor_allocator, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mStaticDescriptors;  // Global descriptors, long lived

    std::unique_ptr<gpu_global_resource_state> mGlobalResourceState;

	static constexpr  int   cMaxFrameCache = 5; // Keep up to 5 frames in the cache
	std::array<std::unique_ptr<gpu_frame_cache>, cMaxFrameCache> mPerFrameCache{};

	[[nodiscard]] inline struct gpu_frame_cache* GetFrameCache() const;
};

struct gpu_frame_cache
{
	GpuState*                 mGlobal = nullptr;
	std::vector<gpu_resource> mStaleResources = {}; // Resources that needs to be freed. Is freed on next use

	gpu_command_list*          mGraphicsList = nullptr;
	gpu_command_list*          mCopyList     = nullptr;
	gpu_command_list*          mComputeList  = nullptr;

    gpu_resource_state_tracker mResourceStateTracker = {};

    // NOTE(Enlynn): ideally we would create this from a scratch memory texture pool, but for now,
    // let's just create one texture per FrameCache
    std::array<gpu_texture, u32(gpu_framebuffer_binding::max)> mFramebuffers{};

    //
    // Convenient Wrapper functions to make accessing state a bit simpler
    //

    [[nodiscard]] gpu_device* GetDevice() const { return mGlobal->mDevice.get(); }

    gpu_texture* GetFramebuffer(gpu_framebuffer_binding Binding) { return &mFramebuffers[u32(Binding)]; }

    void FlushGPU()
    {
        mGlobal->mGraphicsQueue->Flush();
        // TODO(enlynn): Flush the mCompute and Copy Queues
    }

	// Convenience Function to get the active command list
	// For now, just using the Graphics command list
	gpu_command_list* GetGraphicsCommandList() { if (!mGraphicsList) { mGraphicsList = mGlobal->mGraphicsQueue->GetCommandList(); } return mGraphicsList; }
	gpu_command_list* GetCopyCommandList()     { if (!mGraphicsList) { mGraphicsList = mGlobal->mGraphicsQueue->GetCommandList(); } return mGraphicsList; }
	gpu_command_list* GetComputeCommandList()  { if (!mGraphicsList) { mGraphicsList = mGlobal->mGraphicsQueue->GetCommandList(); } return mGraphicsList; }

	// Convenience Function to submit the active command list
	// For now, just using the Graphics command list
	void SubmitGraphicsCommandList() { SubmitGraphicsCommandList(mGraphicsList); mGraphicsList = nullptr; }
	void SubmitCopyCommandList()     { SubmitGraphicsCommandList(mGraphicsList); mGraphicsList = nullptr; }
	void SubmitComputeCommandList()  { SubmitGraphicsCommandList(mGraphicsList); mGraphicsList = nullptr; }

	// Similar to the above functions, but can submit a one-time use command list
	void SubmitGraphicsCommandList(gpu_command_list* CommandList)
    {
        if (CommandList)
        {
            // Get any initial barrier transitions and make sure they happen first
            gpu_command_list* PendingBarriersList = mGlobal->mGraphicsQueue->GetCommandList();
            u32 NumPendingBarriers = mGlobal->mGlobalResourceState->FlushPendingResourceBarriers(*PendingBarriersList, mResourceStateTracker);

            // Submit the command lists
            if (NumPendingBarriers > 0)
            {
                gpu_command_list* ToSubmit[] = {PendingBarriersList, CommandList};
                mGlobal->mGraphicsQueue->ExecuteCommandLists(farray(ToSubmit, 2));
            }
            else
            {
                mGlobal->mGraphicsQueue->SubmitEmptyCommandList(PendingBarriersList); // no barriers recorded.
                mGlobal->mGraphicsQueue->ExecuteCommandLists(farray(&CommandList, 1));
            }

            mGlobal->mGlobalResourceState->SubmitResourceStates(mResourceStateTracker);
        }
    }

    void SubmitCopyCommandList(gpu_command_list* CommandList)     { if (CommandList) { mGlobal->mGraphicsQueue->ExecuteCommandLists(farray(&CommandList, 1)); } }
    void SubmitComputeCommandList(gpu_command_list* CommandList)  { if (CommandList) { mGlobal->mGraphicsQueue->ExecuteCommandLists(farray(&CommandList, 1)); } }

    // Add a stale Resource to the queue to be freed eventually
    void AddStaleResource(gpu_resource Resource)                  { mStaleResources.push_back(Resource); }

    void TrackResource(class gpu_resource& Resource, D3D12_RESOURCE_STATES InitialState, UINT SubResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) const
    {
        mGlobal->mGlobalResourceState->AddResource(Resource, InitialState, SubResource);
    }

    void RemoveTrackedResource(const class gpu_resource& Resource) const
    {
        mGlobal->mGlobalResourceState->RemoveResource(Resource);
    }

    // Push a transition Resource barier
    void TransitionResource(const gpu_resource*   Resource,
                            D3D12_RESOURCE_STATES StateAfter,
                            UINT                  SubResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
    {
        mResourceStateTracker.TransitionBarrier(Resource, StateAfter, SubResource);
    }

    // Push a UAV Barrier for a given Resource
    // @param Resource: if null, then any UAV access could require the barrier
    void UAVBarrier(gpu_resource* Resource = 0) { mResourceStateTracker.UAVBarrier(Resource); }

    // Push an aliasing barrier for a given Resource.
    // @param: both can be null, which indicates that any placed/reserved Resource could cause aliasing
    void AliasBarrier(gpu_resource* ResourceBefore = 0, gpu_resource* ResourceAfter = 0)
    {
        mResourceStateTracker.AliasBarrier(ResourceBefore, ResourceAfter);
    }

    void FlushResourceBarriers(gpu_command_list* CommandList) { mResourceStateTracker.FlushResourceBarriers(CommandList); }

    void releaseStateResources();
};

inline gpu_frame_cache* GpuState::GetFrameCache() const { return mPerFrameCache[mFrameCount % cMaxFrameCache].get(); }
