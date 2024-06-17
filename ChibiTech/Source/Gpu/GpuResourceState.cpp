#include "GpuResourceState.h"
#include "GpuCommandList.h"

#include <Platform/Assert.h>
#include <Platform/Console.h>

static GpuResourceStateMapEntry*
GetResourceMapEntry(GpuResourceStateMap& ResourceMap, ID3D12Resource* Handle)
{
    ForRange (int, i, ResourceMap.size())
    {
        GpuResourceStateMapEntry* Entry = &ResourceMap[i];
        if (Entry->mResourceHandle == Handle) return Entry;
    }

    return nullptr;
}

static void
RemoveResourceMapEntry(GpuResourceStateMap& ResourceMap, ID3D12Resource* Handle)
{
    ForRange (int, i, ResourceMap.size())
    {
        GpuResourceStateMapEntry* Entry = &ResourceMap[i];
        // Order is not preserved in the map, so a Swap-Delete is fine.
        if (Entry->mResourceHandle == Handle) {
            ResourceMap[i] = ResourceMap.back();
            ResourceMap.pop_back();
        }
    }
}

GpuResourceState::GpuResourceState(D3D12_RESOURCE_STATES State)
{
    mState             = State;
    mSubresourcesCount = 0;
}

void GpuResourceState::setSubresourceState(u32 Subresource, D3D12_RESOURCE_STATES State)
{
    // If we are transitioning all resources, then no need to track a subresource
    if (Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
    {
        mState             = State;
        mSubresourcesCount = 0;
        return;
    }

    // Updating a single Resource, let's see if it is already being tracked.
    ForRange(u32, i, mSubresourcesCount)
    {
        if (Subresource == mSubresources[i].mIndex)
        {
            mSubresources[i].mState = State;
            return;
        }
    }

    // Subresource is not found, let's insert it.
    ASSERT(mSubresourcesCount + 1 < cMaxSubresources);

    GpuSubresourceState NewSubresource = {};
    NewSubresource.mIndex = Subresource;
    NewSubresource.mState = State;

    mSubresources[mSubresourcesCount] = NewSubresource;
    mSubresourcesCount += 1;
}

D3D12_RESOURCE_STATES GpuResourceState::getSubresourceState(u32 Subresource)
{
    D3D12_RESOURCE_STATES Result = mState;

    ForRange(u32, i, mSubresourcesCount)
    {
        if (Subresource == mSubresources[i].mIndex)
        {
            Result = mSubresources[i].mState;
        }
    }

    return Result;
}

GpuResourceStateTracker::GpuResourceStateTracker()
{
    mPendingResourceBarriers = std::vector<D3D12_RESOURCE_BARRIER>();
    mPendingResourceBarriers.reserve(10);

    mResourceBarriers        = std::vector<D3D12_RESOURCE_BARRIER>();
    mResourceBarriers.reserve(10);

    mFinalResourceState      = GpuResourceStateMap();
    mFinalResourceState.reserve(10);
}

void GpuResourceStateTracker::deinit()
{
    mPendingResourceBarriers.clear();
    mResourceBarriers.clear();
    mFinalResourceState.clear();
}

// Push a Resource barrier
void GpuResourceStateTracker::resourceBarrier(D3D12_RESOURCE_BARRIER& Barrier)
{
    if (Barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
    { // Transition type barrier
        D3D12_RESOURCE_TRANSITION_BARRIER& TransitionBarrier = Barrier.Transition;

        // Is this a known Barrier, if so, we know the last used state.
        GpuResourceStateMapEntry* KnownResource = GetResourceMapEntry(mFinalResourceState, TransitionBarrier.pResource);
        if (KnownResource)
        {
            // If this is an updated state and ALL_SUBRESOURCES, then transition all known subresources.
            if (TransitionBarrier.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES && KnownResource->mState.mSubresourcesCount > 0)
            {
                ForRange (u32, i, KnownResource->mState.mSubresourcesCount)
                {
                    GpuSubresourceState& SubresourceState = KnownResource->mState.mSubresources[i];
                    if (SubresourceState.mState != TransitionBarrier.StateAfter)
                    {
                        D3D12_RESOURCE_BARRIER NewBarrier = Barrier;
                        NewBarrier.Transition.Subresource = SubresourceState.mIndex;
                        NewBarrier.Transition.StateBefore = SubresourceState.mState;
                        mResourceBarriers.push_back(NewBarrier);
                    }
                }
            }
            else
            { // Transitioning a specific subresource, add a barrier if it's final state is changing
                D3D12_RESOURCE_STATES BeforeState = KnownResource->mState.getSubresourceState(
                        TransitionBarrier.Subresource);
                if (BeforeState != TransitionBarrier.StateAfter)
                {
                    D3D12_RESOURCE_BARRIER NewBarrier = Barrier;
                    NewBarrier.Transition.StateBefore = BeforeState;
                    mResourceBarriers.push_back(NewBarrier);
                }
            }

            // Update the Resource State
            KnownResource->mState.setSubresourceState(TransitionBarrier.Subresource, TransitionBarrier.StateAfter);
        }
        else
        {
            // This is an unknown barrier, add the barrier to a pending List.
            // This list will be resolved before the command list is executed on the GPU.
            mPendingResourceBarriers.push_back(Barrier);

            // The state of the barrier is now "known" at execution. Can add it to our Know Resource states.
            GpuResourceState State{};
            State.setSubresourceState(TransitionBarrier.Subresource, TransitionBarrier.StateAfter);

            GpuResourceStateMapEntry Entry = { .mResourceHandle = TransitionBarrier.pResource, .mState = State };
            mFinalResourceState.push_back(Entry);
        }
    }
    else
    { // Either UAV or Aliasing barrier, just add it to the list
        mResourceBarriers.push_back(Barrier);
    }
}

// Push a transition Resource barier
void GpuResourceStateTracker::transitionBarrier(
        const struct GpuResource *Resource,
        D3D12_RESOURCE_STATES StateAfter,
        UINT                  SubResource)
{
    if (Resource)
    {
        D3D12_RESOURCE_BARRIER Barrier = {};
        Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        Barrier.Transition.pResource   = Resource->asHandle();
        Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        Barrier.Transition.StateAfter  = StateAfter;
        Barrier.Transition.Subresource = SubResource;
        resourceBarrier(Barrier);
    }
}

// Push a UAV Barrier for a given Resource
// @param Resource: if null, then any UAV access could require the barrier
void GpuResourceStateTracker::uavBarrier(const GpuResource* Resource)
{
    D3D12_RESOURCE_BARRIER Barrier = {};
    Barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    Barrier.UAV.pResource = Resource->asHandle();
    resourceBarrier(Barrier);
}

// Push an aliasing barrier for a given Resource.
// @param: both can be null, which indicates that any placed/reserved Resource could cause aliasing
void
GpuResourceStateTracker::aliasBarrier(const GpuResource* ResourceBefore, const GpuResource* ResourceAfter)
{
    D3D12_RESOURCE_BARRIER Barrier = {};
    Barrier.Type                     = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    Barrier.Aliasing.pResourceBefore = ResourceBefore ? ResourceBefore->asHandle() : nullptr;
    Barrier.Aliasing.pResourceAfter  = ResourceAfter  ? ResourceAfter->asHandle()  : nullptr;
    resourceBarrier(Barrier);
}

// flush any (non-pending) Resource barriers that have been pushed to the Resource state tracker.
void GpuResourceStateTracker::flushResourceBarriers(struct GpuCommandList *CommandList)
{
    UINT NumBarriers = (UINT)mResourceBarriers.size();
    if (NumBarriers > 0)
    {
        CommandList->asHandle()->ResourceBarrier(NumBarriers, mResourceBarriers.data());
        mResourceBarriers.clear();
    }
}

// clear Resource state tracking. Done when command list is reset.
void GpuResourceStateTracker::reset()
{
    ASSERT(mPendingResourceBarriers.size() == 0); // This should be handled before submitting the command list.
    ASSERT(mResourceBarriers.size() == 0);        // Extra barriers submitted that we didn't have to
    mFinalResourceState.clear();                    // There is no known Resource states anymore.
}

void GpuGlobalResourceState::submitResourceStates(GpuResourceStateTracker& StateTracker)
{
    const GpuResourceStateMap& FinalResourceStates = StateTracker.getFinalResourceState();
    for (const auto& State : FinalResourceStates)
    {
        GpuResourceStateMapEntry* KnownResource = GetResourceMapEntry(mKnownStates, State.mResourceHandle);
        if (KnownResource)
            *KnownResource = State;
        else
        {
            GpuResourceStateMapEntry NewState = State;
            mKnownStates.push_back(NewState);
        }
    }
}

void GpuGlobalResourceState::addResource(struct GpuResource &Resource, D3D12_RESOURCE_STATES InitialState, UINT SubResource)
{
    GpuResourceStateMapEntry* KnownResource = GetResourceMapEntry(mKnownStates, Resource.asHandle());
    if (KnownResource)
    {
        // This is a known Resource, we can't override the existing state
        if (SubResource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
        {
            ct::console::warn("Attempting to add a Resource to the global state map, but Resource exists. Replacing old Resource");
            return;
        }

        // Is this one of the subresources? If so, can't override the existing subresource
        ForRange(u32, i, KnownResource->mState.mSubresourcesCount)
        {
            GpuSubresourceState& SubresourceState = KnownResource->mState.mSubresources[i];
            if (SubresourceState.mIndex == SubResource)
            {
                ct::console::warn("Attempting to add a Resource to the global state map, but Resource exists. Replacing old Resource");
                return;
            }
        }

        // Add the subresource, but not if it exceeds the allowed amount of subresources
        KnownResource->mState.setSubresourceState(SubResource, InitialState);
    }
    else
    {
        GpuResourceState NewState{};
        NewState.setSubresourceState(SubResource, InitialState);

        GpuResourceStateMapEntry NewEntry = {};
        NewEntry.mState          = NewState;
        NewEntry.mResourceHandle = Resource.asHandle();

        mKnownStates.push_back(NewEntry);
    }
}

void GpuGlobalResourceState::removeResource(const struct GpuResource &Resource)
{
    RemoveResourceMapEntry(mKnownStates, Resource.asHandle());
}

// flush any pending Resource barriers to the command list
u32 GpuGlobalResourceState::flushPendingResourceBarriers(struct GpuCommandList &CommandList, GpuResourceStateTracker& StateTracker)
{
    const std::vector<D3D12_RESOURCE_BARRIER>& PendingBarriers = StateTracker.getPendingBarriers();

    std::vector<D3D12_RESOURCE_BARRIER> BarriersToSubmit = std::vector<D3D12_RESOURCE_BARRIER>();
    BarriersToSubmit.reserve(PendingBarriers.size());

    for (const auto& Barrier : PendingBarriers)
    {
        ASSERT(Barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION);

        const D3D12_RESOURCE_TRANSITION_BARRIER& TransitionBarrier = Barrier.Transition;

        // Is this a known Barrier, if so, we know the last used state.
        GpuResourceStateMapEntry* KnownResource = GetResourceMapEntry(mKnownStates, TransitionBarrier.pResource);
        if (KnownResource)
        { // If this is an updated state and ALL_SUBRESOURCES, then transition all known subresources.
            if (TransitionBarrier.Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
            {
                if (KnownResource->mState.mSubresourcesCount == 0)
                {
                    if (KnownResource->mState.mState != TransitionBarrier.StateAfter)
                    {
                        D3D12_RESOURCE_BARRIER NewBarrier = Barrier;
                        NewBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                        NewBarrier.Transition.StateBefore = KnownResource->mState.mState;
                        BarriersToSubmit.push_back(NewBarrier);
                    }
                }
                else
                {
                    ForRange(u32, i, KnownResource->mState.mSubresourcesCount)
                    {
                        GpuSubresourceState& SubresourceState = KnownResource->mState.mSubresources[i];
                        if (SubresourceState.mState != TransitionBarrier.StateAfter)
                        {
                            D3D12_RESOURCE_BARRIER NewBarrier = Barrier;
                            NewBarrier.Transition.Subresource = SubresourceState.mIndex;
                            NewBarrier.Transition.StateBefore = SubresourceState.mState;
                            BarriersToSubmit.push_back(NewBarrier);
                        }
                    }
                }
            }
            else
            { // Transitioning a specific subresource, add a barrier if it's final state is changing
                D3D12_RESOURCE_STATES BeforeState =
                        KnownResource->mState.getSubresourceState(TransitionBarrier.Subresource);
                if (BeforeState != TransitionBarrier.StateAfter)
                {
                    D3D12_RESOURCE_BARRIER NewBarrier = Barrier;
                    NewBarrier.Transition.StateBefore = BeforeState;
                    BarriersToSubmit.push_back(NewBarrier);
                }
            }

            // Update the Resource State
            //KnownResource->mState.setSubresourceState(transitionBarrier.Subresource, transitionBarrier.StateAfter);
        }
        else
        {
            // This is an unknown barrier, add the barrier to a pending List.
            // This list will be resolved before the command list is executed on the GPU.
            D3D12_RESOURCE_BARRIER NewBarrier = Barrier;
            BarriersToSubmit.push_back(NewBarrier);

            // The state of the barrier is now "known" at execution. Can add it to our Know Resource states.
            GpuResourceState State{};
            State.setSubresourceState(TransitionBarrier.Subresource, TransitionBarrier.StateAfter);

            GpuResourceStateMapEntry Entry = { .mResourceHandle = TransitionBarrier.pResource, .mState = State };
            mKnownStates.push_back(Entry);
        }
    }

    // Submit the barriers.
    UINT NumBarriers = (UINT)BarriersToSubmit.size();
    if (NumBarriers > 0)
    {
        CommandList.asHandle()->ResourceBarrier(NumBarriers, BarriersToSubmit.data());
    }

    // No more pending barriers, so clear the list.
    StateTracker.clearPendingBarriers();

    return NumBarriers;
}