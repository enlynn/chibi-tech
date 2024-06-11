
#pragma once

#include "D3D12Common.h"

#include <vector>

struct gpu_subresource_state
{
    u32                   mIndex = 0;
    D3D12_RESOURCE_STATES mState = D3D12_RESOURCE_STATE_COMMON;
};

struct gpu_resource_state
{
    //gpu_resource_state() = default;
    gpu_resource_state(D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON);

    void SetSubresourceState(u32 Subresource, D3D12_RESOURCE_STATES State = D3D12_RESOURCE_STATE_COMMON);
    D3D12_RESOURCE_STATES GetSubresourceState(u32 Subresource);

    D3D12_RESOURCE_STATES mState                          = D3D12_RESOURCE_STATE_COMMON;

    // TODO(enlynn): Not entirely sure what a reasonable upper bound for subresources would be. For now
    // going to just declare 10 and see how it goes. If it much more than this, will likely need a dynamic array.
    static constexpr u32 cMaxSubresources                 = 10;
    gpu_subresource_state mSubresources[cMaxSubresources] = {};
    u32                   mSubresourcesCount              = 0;
};

struct gpu_resource_state_map_entry
{
    ID3D12Resource*    mResourceHandle = nullptr;
    gpu_resource_state mState          = {};
};

using gpu_resource_state_map = std::vector<gpu_resource_state_map_entry>;

class gpu_resource_state_tracker
{
public:
    gpu_resource_state_tracker();
    void Deinit();

    // Push a Resource barrier
    void ResourceBarrier(D3D12_RESOURCE_BARRIER& Barrier);

    // Push a transition Resource barier
    void TransitionBarrier(const class gpu_resource* Resource,
                           D3D12_RESOURCE_STATES     StateAfter,
                           UINT                      SubResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

    // Push a UAV Barrier for a given Resource
    // @param Resource: if null, then any UAV access could require the barrier
    void UAVBarrier(const class gpu_resource* Resource = 0);

    // Push an aliasing barrier for a given Resource.
    // @param: both can be null, which indicates that any placed/reserved Resource could cause aliasing
    void AliasBarrier(const class gpu_resource* ResourceBefore = 0, const class gpu_resource* ResourceAfter = 0);

    // Flush any (non-pending) Resource barriers that have been pushed to the Resource state tracker.
    void FlushResourceBarriers(class gpu_command_list* CommandList);

    // Reset Resource state tracking. Done when command list is reset.
    void Reset();

    const std::vector<D3D12_RESOURCE_BARRIER>& GetPendingBarriers()   const { return mPendingResourceBarriers; }
    void                                       ClearPendingBarriers()       { mPendingResourceBarriers.clear(); }

    const gpu_resource_state_map& GetFinalResourceState()   const { return mFinalResourceState;  }
    void                          ClearFinalResourceState()       { mFinalResourceState.clear(); }

private:
    std::vector<D3D12_RESOURCE_BARRIER> mPendingResourceBarriers = {};
    std::vector<D3D12_RESOURCE_BARRIER> mResourceBarriers        = {};
    gpu_resource_state_map              mFinalResourceState      = {};
};

struct gpu_global_resource_state
{
    gpu_resource_state_map mKnownStates;

    // Submit known Resource state to the global tracker.
    void SubmitResourceStates(gpu_resource_state_tracker& StateTracker);
    // Flush any pending Resource barriers to the command list
    u32 FlushPendingResourceBarriers(class gpu_command_list& CommandList, gpu_resource_state_tracker& StateTracker);

    void AddResource(class gpu_resource& Resource, D3D12_RESOURCE_STATES InitialState, UINT SubResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    void RemoveResource(const class gpu_resource& Resource);
};
