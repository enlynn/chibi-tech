
#pragma once

#include "D3D12Common.h"

#include <vector>

struct GpuSubresourceState
{
    u32                   mIndex = 0;
    D3D12_RESOURCE_STATES mState = D3D12_RESOURCE_STATE_COMMON;
};

struct GpuResourceState
{
    //GpuResourceState() = default;
    explicit GpuResourceState(D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON);

    void setSubresourceState(u32 Subresource, D3D12_RESOURCE_STATES State = D3D12_RESOURCE_STATE_COMMON);
    D3D12_RESOURCE_STATES getSubresourceState(u32 Subresource);

    D3D12_RESOURCE_STATES mState                          = D3D12_RESOURCE_STATE_COMMON;

    // TODO(enlynn): Not entirely sure what a reasonable upper bound for subresources would be. For now
    // going to just declare 10 and see how it goes. If it much more than this, will likely need a Dynamic array.
    static constexpr u32 cMaxSubresources                 = 10;
    GpuSubresourceState   mSubresources[cMaxSubresources] = {};
    u32                   mSubresourcesCount              = 0;
};

struct GpuResourceStateMapEntry
{
    ID3D12Resource*  mResourceHandle{nullptr};
    GpuResourceState mState{};
};

using GpuResourceStateMap = std::vector<GpuResourceStateMapEntry>;

class GpuResourceStateTracker
{
public:
    GpuResourceStateTracker();
    void deinit();

    // Push a Resource barrier
    void resourceBarrier(D3D12_RESOURCE_BARRIER& Barrier);

    // Push a transition Resource barier
    void transitionBarrier(const class GpuResource *Resource,
                           D3D12_RESOURCE_STATES     StateAfter,
                           UINT                      SubResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

    // Push a UAV Barrier for a given Resource
    // @param Resource: if null, then any UAV access could require the barrier
    void uavBarrier(const class GpuResource* Resource = 0);

    // Push an aliasing barrier for a given Resource.
    // @param: both can be null, which indicates that any placed/reserved Resource could cause aliasing
    void aliasBarrier(const class GpuResource* ResourceBefore = 0, const class GpuResource* ResourceAfter = 0);

    // flush any (non-pending) Resource barriers that have been pushed to the Resource state tracker.
    void flushResourceBarriers(struct GpuCommandList *CommandList);

    // reset Resource state tracking. Done when command list is reset.
    void reset();

    const std::vector<D3D12_RESOURCE_BARRIER>& getPendingBarriers()   const { return mPendingResourceBarriers; }
    void                                       clearPendingBarriers()       { mPendingResourceBarriers.clear(); }

    const GpuResourceStateMap& getFinalResourceState()   const { return mFinalResourceState;  }
    void                       clearFinalResourceState()       { mFinalResourceState.clear(); }

private:
    std::vector<D3D12_RESOURCE_BARRIER> mPendingResourceBarriers = {};
    std::vector<D3D12_RESOURCE_BARRIER> mResourceBarriers        = {};
    GpuResourceStateMap                 mFinalResourceState      = {};
};

struct GpuGlobalResourceState
{
    GpuResourceStateMap mKnownStates;

    // Submit known Resource state to the global tracker.
    void submitResourceStates(GpuResourceStateTracker& StateTracker);
    // flush any pending Resource barriers to the command list
    u32 flushPendingResourceBarriers(struct GpuCommandList &CommandList, GpuResourceStateTracker& StateTracker);

    void addResource(struct GpuResource &Resource, D3D12_RESOURCE_STATES InitialState, UINT SubResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    void removeResource(const struct GpuResource &Resource);
};
