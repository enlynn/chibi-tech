#pragma once

#include "gpu_command_list.h"

#include <deque>

#include <Util/array.h>

class gpu_device;
class gpu_command_list;

struct ID3D12CommandQueue;
struct ID3D12Fence;

enum class gpu_command_queue_type : u8
{
	none,     //default init state, not valid to use
	graphics,
	compute,
	copy
};

enum class gpu_fence_result
{
	success,
};

class gpu_command_queue
{
public:
	gpu_command_queue() = default;
	gpu_command_queue(gpu_command_queue_type Type, gpu_device *Device);
	~gpu_command_queue() { Deinit(); }

	inline ID3D12CommandQueue* AsHandle() const { return mQueueHandle; }

	void              Deinit();

	void              Flush(); // flushes the command list so that no work is being done

	// Command Lists
	gpu_command_list* GetCommandList(gpu_command_list_type Type = gpu_command_list_type::graphics);
	u64               ExecuteCommandLists(farray<gpu_command_list*> CommandLists);
	void              ProcessCommandLists(); // Call once per frame, checks if in-flight commands lists are completed
    void              SubmitEmptyCommandList(gpu_command_list* CommandList);


	// Synchronization
	u64               Signal();                                  // (nonblocking) signals the fence and returns its current value
	bool              IsFenceComplete(u64 FenceValue);           // (nonblocking) check the currnt value of a fence
	gpu_fence_result  WaitForFence(u64 FenceValue);              // (blocking)    wait for a fence to have passed value
	void              Wait(const gpu_command_queue* OtherQueue); // (blocking)    wait for another command queue to finish executing

private:
	gpu_device*               mDevice       = nullptr;
	gpu_command_queue_type    mType         = gpu_command_queue_type::none;

	ID3D12CommandQueue*       mQueueHandle   = nullptr;

	// Synchronization
	ID3D12Fence*              mQueueFence   = nullptr;
	u64                       mFenceValue   = 0;

	// TODO:
	// - Determine if creating command lists on the fly is cheap
	// - Determine if command lists can be used after submitting
	// - Determine Resource ownsership. Previously command lists were used as sudo-GC.
	struct in_flight_list
	{
		gpu_command_list* CmdList;
		u64               FenceValue;
	};

	std::vector<in_flight_list>    mInFlightCommandLists                                           = {};
    std::deque<gpu_command_list*> mAvailableFlightCommandLists[u32(gpu_command_list_type::count)] = {}; // slot for each array type

	//std::deque<in_flight_list>    mInFlightCommandLists = {};
	//std::deque<gpu_command_list*> mAvailableFlightCommandLists[u32(gpu_command_list_type::Count)] = {};

};