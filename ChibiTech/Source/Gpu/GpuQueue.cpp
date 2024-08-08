#include "D3D12Common.h"
#include "GpuQueue.h"
#include "GpuDevice.h"

#include <Platform/Assert.h>

#include <vector>

inline D3D12_COMMAND_LIST_TYPE
GetD3D12CommandListType(GpuCommandQueueType Type)
{
#if 0
D3D12_COMMAND_LIST_TYPE_DIRECT
D3D12_COMMAND_LIST_TYPE_BUNDLE
D3D12_COMMAND_LIST_TYPE_COMPUTE
D3D12_COMMAND_LIST_TYPE_COPY
D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE
D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS
D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE
D3D12_COMMAND_LIST_TYPE_NONE
#endif

	switch (Type)
	{
		case GpuCommandQueueType::Graphics: return D3D12_COMMAND_LIST_TYPE_DIRECT;
		case GpuCommandQueueType::Compute:  return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		case GpuCommandQueueType::Copy:     return D3D12_COMMAND_LIST_TYPE_COPY;
		default:                               return D3D12_COMMAND_LIST_TYPE_NONE;
	}
}

GpuQueue::GpuQueue(GpuCommandQueueType Type, GpuDevice *Device)
	: mDevice(Device)
	, mType(Type)
	, mFenceValue(0)
{
	D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
	QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	QueueDesc.Type = GetD3D12CommandListType(mType);
	AssertHr(mDevice->asHandle()->CreateCommandQueue(&QueueDesc, ComCast(&mQueueHandle)));

	AssertHr(mDevice->asHandle()->CreateFence(mFenceValue, D3D12_FENCE_FLAG_NONE, ComCast(&mQueueFence)));

	ForRange(u32, i, u32(GpuCommandListType::Count))
	{
		mAvailableFlightCommandLists[i] = std::deque<GpuCommandList*>();
	}

	mInFlightCommandLists = std::vector<InFlightList>();
}

void 
GpuQueue::deinit()
{
	if (!mQueueHandle || !mQueueFence) return; // nothing to deinit

    flush();
	ASSERT(mInFlightCommandLists.empty());

	mInFlightCommandLists.clear();

	u32 ListTypeCount = u32(GpuCommandListType::Count);
	ForRange(u32, i, ListTypeCount)
	{
		ForRange(u32, j, mAvailableFlightCommandLists[i].size())
		{
			GpuCommandList* List = mAvailableFlightCommandLists[i][j];
			List->release();

			delete List;
		}

		mAvailableFlightCommandLists[i].clear();
	}

	ComSafeRelease(mQueueFence);
	ComSafeRelease(mQueueHandle);

	mDevice = nullptr;
}

void GpuQueue::flush()
{
	do
	{
        waitForFence(mFenceValue);
        processCommandLists();
		// We should not execute more than a single instance of this loop, but just in case..
	} while (!mInFlightCommandLists.empty());
}

GpuCommandList*
GpuQueue::getCommandList(GpuCommandListType Type)
{
	ASSERT(Type != GpuCommandListType::None);
	u32 TypeIndex = u32(Type);

	if (mAvailableFlightCommandLists[TypeIndex].size() > 0)
	{
        GpuCommandList* Result = mAvailableFlightCommandLists[TypeIndex][u64(0)]; // The List was reset when it became available.
		mAvailableFlightCommandLists[TypeIndex].pop_front();

        Result->reset(); // Let the command list free any resources it was holding onto
        return Result;
	}
	else
	{
		return new GpuCommandList(*mDevice, Type);
	}
}

u64               
GpuQueue::executeCommandLists(farray<GpuCommandList*> CommandLists)
{
	// TODO(enlynn): Originally...I had to submit a copy for every command list
	// in order to correctly resolve Resource state. This time around, I would
	// like this process to be done manually (through a RenderPass perhaps?) instead
	// of automatically.

    std::vector<ID3D12CommandList*> ToBeSubmitted{};
    ToBeSubmitted.resize(CommandLists.Length());

	ForRange(u64, i, CommandLists.Length())
	{
        CommandLists[i]->close();
		ToBeSubmitted[i] = CommandLists[i]->asHandle();
	}

	// TODO(enlynn): MipMaps

	mQueueHandle->ExecuteCommandLists((UINT)ToBeSubmitted.size(), ToBeSubmitted.data());
	u64 NextFenceValue = signal();

	ForRange(u64, i, CommandLists.Length())
	{
        for (const auto& InFlight : mInFlightCommandLists)
        {
            ASSERT(InFlight.CmdList != CommandLists[i]);
        } 

		InFlightList InFlight = { .CmdList = CommandLists[i] , .FenceValue = NextFenceValue };
		mInFlightCommandLists.push_back(InFlight);
	}

	ToBeSubmitted.clear();

	return NextFenceValue;
}

void GpuQueue::submitEmptyCommandList(GpuCommandList* CommandList)
{
    CommandList->close();
    mAvailableFlightCommandLists[u32(CommandList->getType())].push_back(CommandList);
}

void              
GpuQueue::processCommandLists()
{
    int iterations = 0;
    GpuCommandList* OldList = nullptr;

	while (mInFlightCommandLists.size() > 0 && isFenceComplete(mInFlightCommandLists[u64(0)].FenceValue))
	{
		InFlightList& List = mInFlightCommandLists[u64(0)];

		mAvailableFlightCommandLists[u32(List.CmdList->getType())].push_back(List.CmdList);

        OldList = List.CmdList;
		mInFlightCommandLists.erase(mInFlightCommandLists.begin());

        iterations += 1;
	}
}

u64 
GpuQueue::signal()
{
	mFenceValue += 1;
	AssertHr(mQueueHandle->Signal(mQueueFence, mFenceValue));
	return mFenceValue;
}

bool           
GpuQueue::isFenceComplete(u64 FenceValue)
{
	return mQueueFence->GetCompletedValue() >= FenceValue;
}

GpuFenceResult
GpuQueue::waitForFence(u64 FenceValue)
{
	if (mQueueFence->GetCompletedValue() < FenceValue)
	{
        HANDLE Event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		AssertHr(mQueueFence->SetEventOnCompletion(FenceValue, Event));
		WaitForSingleObject(Event, INFINITE);
		::CloseHandle(Event);
	}

	// TODO(enlynn): Would it be worth updating the fence value here?

	return GpuFenceResult::success;
}

void             
GpuQueue::wait(const GpuQueue* OtherQueue)
{
	mQueueHandle->Wait(OtherQueue->mQueueFence, OtherQueue->mFenceValue);
}
