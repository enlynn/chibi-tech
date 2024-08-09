#include "D3D12Common.h"
#include "GpuQueue.h"
#include "GpuDevice.h"

#include <Platform/Assert.h>

#include <vector>

inline D3D12_COMMAND_LIST_TYPE
GetD3D12CommandListType(GpuQueueType Type)
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
		case GpuQueueType::Graphics: return D3D12_COMMAND_LIST_TYPE_DIRECT;
		case GpuQueueType::Compute:  return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		case GpuQueueType::Copy:     return D3D12_COMMAND_LIST_TYPE_COPY;
		default:                     return D3D12_COMMAND_LIST_TYPE_NONE;
	}
}

GpuQueue::GpuQueue(GpuQueueType Type, GpuDevice *Device)
	: mDevice(Device)
	, mType(Type)
{
	mFenceValue.mType  = u64(Type);
	mFenceValue.mFence = 0;

	D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
	QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	QueueDesc.Type = GetD3D12CommandListType(mType);
	AssertHr(mDevice->asHandle()->CreateCommandQueue(&QueueDesc, ComCast(&mQueueHandle)));

	AssertHr(mDevice->asHandle()->CreateFence(mFenceValue.mFence, D3D12_FENCE_FLAG_NONE, ComCast(&mQueueFence)));

	ForRange(u32, i, u32(GpuCommandListType::Count))
	{
		mAvailableFlightCommandLists[i] = std::deque<GpuCommandListUPtr>();
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
			GpuCommandListUPtr list = std::move(mAvailableFlightCommandLists[i][j]);
			list->release();

			//unique_ptr is freed as it leaves scope
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

GpuCommandListUPtr GpuQueue::getCommandList(GpuCommandListType Type)
{
	ASSERT(Type != GpuCommandListType::None);
	u32 TypeIndex = u32(Type);

	if (mAvailableFlightCommandLists[TypeIndex].size() > 0)
	{
        GpuCommandListUPtr result = std::move(mAvailableFlightCommandLists[TypeIndex][u64(0)]); // The List was reset when it became available.
		mAvailableFlightCommandLists[TypeIndex].pop_front();

		result->reset(); // Let the command list free any resources it was holding onto
        return result;
	}
	else
	{
		return std::make_unique<GpuCommandList>(*mDevice, Type);
	}
}

GpuFence GpuQueue::executeCommandLists(std::span<GpuCommandListUPtr> tCommandLists)
{
    std::vector<ID3D12CommandList*> toBeSubmitted;
	toBeSubmitted.reserve(tCommandLists.size());

	for(auto& list : tCommandLists)
	{
		list->close();
		toBeSubmitted.push_back(list->asHandle());
	}

	mQueueHandle->ExecuteCommandLists((UINT)toBeSubmitted.size(), toBeSubmitted.data());
	GpuFence nextFenceValue = signal();

	for (size_t i = 0; i < tCommandLists.size(); ++i)
	{
        for (const auto& InFlight : mInFlightCommandLists)
        { // fixme: Debug check, is not necessary
            ASSERT(InFlight.CmdList != tCommandLists[i]);
        }

		InFlightList InFlight = { .CmdList = std::move(tCommandLists[i]), .FenceValue = nextFenceValue};
		mInFlightCommandLists.push_back(std::move(InFlight));
	}

	return nextFenceValue;
}

void GpuQueue::submitEmptyCommandList(GpuCommandListUPtr tCommandList)
{
    tCommandList->close();
    mAvailableFlightCommandLists[u32(tCommandList->getType())].push_back(std::move(tCommandList));
}

void GpuQueue::processCommandLists()
{
    int iterations = 0;
	while (mInFlightCommandLists.size() > 0 && isFenceComplete(mInFlightCommandLists[u64(0)].FenceValue))
	{
		InFlightList& inFlightList = mInFlightCommandLists[u64(0)];

		mAvailableFlightCommandLists[u32(inFlightList.CmdList->getType())].push_back(std::move(inFlightList.CmdList));
		mInFlightCommandLists.erase(mInFlightCommandLists.begin());

        iterations += 1;
	}
}

GpuFence GpuQueue::signal()
{
	mFenceValue.mFence += 1;
	AssertHr(mQueueHandle->Signal(mQueueFence, mFenceValue.mFence));
	return mFenceValue;
}

bool GpuQueue::isFenceComplete(GpuFence tFenceValue)
{
	return mQueueFence->GetCompletedValue() >= tFenceValue.mFence;
}

bool GpuQueue::waitForFence(GpuFence tFenceValue)
{
	if (mQueueFence->GetCompletedValue() < mFenceValue.mFence)
	{
        HANDLE Event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		AssertHr(mQueueFence->SetEventOnCompletion(mFenceValue.mFence, Event));
		WaitForSingleObject(Event, INFINITE);
		::CloseHandle(Event);
	}

	// TODO(enlynn): Would it be worth updating the fence value here?

	return true;
}

void GpuQueue::wait(const GpuQueue* tOtherQueue)
{
	mQueueHandle->Wait(tOtherQueue->mQueueFence, tOtherQueue->mFenceValue.mFence);
}
