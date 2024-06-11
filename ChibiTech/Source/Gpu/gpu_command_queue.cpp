#include "D3D12Common.h"
#include "gpu_command_queue.h"
#include "gpu_device.h"

#include <Platform/Assert.h>

#include <vector>

inline D3D12_COMMAND_LIST_TYPE
GetD3D12CommandListType(gpu_command_queue_type Type)
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
		case gpu_command_queue_type::graphics: return D3D12_COMMAND_LIST_TYPE_DIRECT;
		case gpu_command_queue_type::compute:  return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		case gpu_command_queue_type::copy:     return D3D12_COMMAND_LIST_TYPE_COPY;
		default:                               return D3D12_COMMAND_LIST_TYPE_NONE;
	}
}

gpu_command_queue::gpu_command_queue(gpu_command_queue_type Type, gpu_device *Device)
	: mDevice(Device)
	, mType(Type)
	, mFenceValue(0)
{
	D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
	QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	QueueDesc.Type = GetD3D12CommandListType(mType);
	AssertHr(mDevice->AsHandle()->CreateCommandQueue(&QueueDesc, ComCast(&mQueueHandle)));

	AssertHr(mDevice->AsHandle()->CreateFence(mFenceValue, D3D12_FENCE_FLAG_NONE, ComCast(&mQueueFence)));

	ForRange(u32, i, u32(gpu_command_list_type::count))
	{
		mAvailableFlightCommandLists[i] = std::deque<gpu_command_list*>();
	}

	mInFlightCommandLists = std::vector<in_flight_list>();
}

void 
gpu_command_queue::Deinit()
{
	if (!mQueueHandle || !mQueueFence) return; // nothing to deinit

	Flush();
	ASSERT(mInFlightCommandLists.empty());

	mInFlightCommandLists.clear();

	u32 ListTypeCount = u32(gpu_command_list_type::count);
	ForRange(u32, i, ListTypeCount)
	{
		ForRange(u32, j, mAvailableFlightCommandLists[i].size())
		{
			gpu_command_list* List = mAvailableFlightCommandLists[i][j];
			List->Release();

			delete List;
		}

		mAvailableFlightCommandLists[i].clear();
	}

	ComSafeRelease(mQueueFence);
	ComSafeRelease(mQueueHandle);

	mDevice = nullptr;
}

void gpu_command_queue::Flush()
{
	do
	{
		WaitForFence(mFenceValue);
		ProcessCommandLists();
		// We should not execute more than a single instance of this loop, but just in case..
	} while (!mInFlightCommandLists.empty());
}

gpu_command_list* 
gpu_command_queue::GetCommandList(gpu_command_list_type Type)
{
	ASSERT(Type != gpu_command_list_type::none);
	u32 TypeIndex = u32(Type);

	if (mAvailableFlightCommandLists[TypeIndex].size() > 0)
	{
        gpu_command_list* Result = mAvailableFlightCommandLists[TypeIndex][u64(0)]; // The List was reset when it became available.
		mAvailableFlightCommandLists[TypeIndex].pop_front();

        Result->Reset(); // Let the command list free any resources it was holding onto
        return Result;
	}
	else
	{
		return new gpu_command_list(*mDevice, Type);
	}
}

u64               
gpu_command_queue::ExecuteCommandLists(farray<gpu_command_list*> CommandLists)
{
	// TODO(enlynn): Originally...I had to submit a copy for every command list
	// in order to correctly resolve Resource state. This time around, I would
	// like this process to be done manually (through a RenderPass perhaps?) instead
	// of automatically.

    std::vector<ID3D12CommandList*> ToBeSubmitted{};
    ToBeSubmitted.resize(CommandLists.Length());

	ForRange(u64, i, CommandLists.Length())
	{
		CommandLists[i]->Close();
		ToBeSubmitted[i] = CommandLists[i]->AsHandle();
	}

	// TODO(enlynn): MipMaps

	mQueueHandle->ExecuteCommandLists((UINT)ToBeSubmitted.size(), ToBeSubmitted.data());
	u64 NextFenceValue = Signal();

	ForRange(u64, i, CommandLists.Length())
	{
        for (const auto& InFlight : mInFlightCommandLists)
        {
            ASSERT(InFlight.CmdList != CommandLists[i]);
        } 

		in_flight_list InFlight = { .CmdList = CommandLists[i] , .FenceValue = NextFenceValue };
		mInFlightCommandLists.push_back(InFlight);
	}

	ToBeSubmitted.clear();

	return NextFenceValue;
}

void gpu_command_queue::SubmitEmptyCommandList(gpu_command_list* CommandList)
{
    CommandList->Close();
    mAvailableFlightCommandLists[u32(CommandList->GetType())].push_back(CommandList);
}

void              
gpu_command_queue::ProcessCommandLists()
{
    int iterations = 0;
    gpu_command_list* OldList = nullptr;

	while (mInFlightCommandLists.size() > 0 && IsFenceComplete(mInFlightCommandLists[u64(0)].FenceValue))
	{
		in_flight_list& List = mInFlightCommandLists[u64(0)];

		mAvailableFlightCommandLists[u32(List.CmdList->GetType())].push_back(List.CmdList);

        OldList = List.CmdList;
		mInFlightCommandLists.erase(mInFlightCommandLists.begin());

        iterations += 1;
	}
}

u64 
gpu_command_queue::Signal()
{
	mFenceValue += 1;
	AssertHr(mQueueHandle->Signal(mQueueFence, mFenceValue));
	return mFenceValue;
}

bool           
gpu_command_queue::IsFenceComplete(u64 FenceValue)
{
	return mQueueFence->GetCompletedValue() >= FenceValue;
}

gpu_fence_result 
gpu_command_queue::WaitForFence(u64 FenceValue)
{
	if (mQueueFence->GetCompletedValue() < FenceValue)
	{
        HANDLE Event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		AssertHr(mQueueFence->SetEventOnCompletion(FenceValue, Event));
		WaitForSingleObject(Event, INFINITE);
		::CloseHandle(Event);
	}

	// TODO(enlynn): Would it be worth updating the fence value here?

	return gpu_fence_result::success;
}

void             
gpu_command_queue::Wait(const gpu_command_queue* OtherQueue)
{
	mQueueHandle->Wait(OtherQueue->mQueueFence, OtherQueue->mFenceValue);
}
