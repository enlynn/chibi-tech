#pragma once

#include "GpuCommandList.h"

#include <deque>
#include <span>

#include <Util/array.h>

class GpuDevice;
class GpuCommandList;

struct ID3D12CommandQueue;
struct ID3D12Fence;

enum class GpuQueueType : u8
{
	None,     //default init state, not valid to use
	Graphics,
	Compute,
	Copy
};

union GpuFence {
	struct {
		u64 mType  : 2; // 00 (0 - None), 01 (1 - Graphics), 10 (2 - Compute), 11 (3 - Copy),  
		u64 mFence : 62;
	};
	u64 value;
};

static_assert(sizeof(GpuFence) == sizeof(u64));

class GpuQueue
{
public:
	GpuQueue() = default;
	GpuQueue(GpuQueueType Type, GpuDevice *Device);
	~GpuQueue() { deinit(); }

	inline ID3D12CommandQueue* asHandle() const { return mQueueHandle; }

	void              deinit();

	void              flush(); // flushes the command list so that no work is being done

	// Command Lists
	GpuCommandListUPtr getCommandList(GpuCommandListType Type = GpuCommandListType::Graphics);
	GpuFence           executeCommandLists(std::span<GpuCommandListUPtr> CommandLists);
	void               processCommandLists(); // Call once per frame, checks if in-flight commands lists are completed
    void               submitEmptyCommandList(GpuCommandListUPtr CommandList);


	// Synchronization
	GpuFence          signal();                              // (nonblocking) signals the fence and returns its current value
	bool              isFenceComplete(GpuFence tFenceValue); // (nonblocking) check the currnt value of a fence
	bool              waitForFence(GpuFence tFenceValue);    // (blocking)    wait for a fence to have passed value
	void              wait(const GpuQueue* OtherQueue);      // (blocking)    wait for another command queue to finish executing

private:
	GpuDevice*          mDevice        = nullptr;
	GpuQueueType        mType          = GpuQueueType::None;

	ID3D12CommandQueue* mQueueHandle   = nullptr;

	// Synchronization
	ID3D12Fence*        mQueueFence   = nullptr;
	GpuFence            mFenceValue   = {};

	// TODO:
	// - Determine if creating command lists on the fly is cheap
	// - Determine if command lists can be used after submitting
	// - Determine Resource ownsership. Previously command lists were used as sudo-GC.
	struct InFlightList
	{
		GpuCommandListUPtr CmdList;
		GpuFence           FenceValue;
	};

	std::vector<InFlightList>      mInFlightCommandLists                                        = {};
    std::deque<GpuCommandListUPtr> mAvailableFlightCommandLists[u32(GpuCommandListType::Count)] = {}; // slot for each array type

	//std::deque<InFlightList>    mInFlightCommandLists = {};
	//std::deque<GpuCommandList*> mAvailableFlightCommandLists[u32(GpuCommandListType::Count)] = {};

};