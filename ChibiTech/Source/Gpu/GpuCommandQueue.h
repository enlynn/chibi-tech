#pragma once

#include "GpuCommandList.h"

#include <deque>

#include <Util/array.h>

class GpuDevice;
class GpuCommandList;

struct ID3D12CommandQueue;
struct ID3D12Fence;

enum class GpuCommandQueueType : u8
{
	None,     //default init state, not valid to use
	Graphics,
	Compute,
	Copy
};

enum class GpuFenceResult
{
	success,
};

class GpuCommandQueue
{
public:
	GpuCommandQueue() = default;
	GpuCommandQueue(GpuCommandQueueType Type, GpuDevice *Device);
	~GpuCommandQueue() { deinit(); }

	inline ID3D12CommandQueue* asHandle() const { return mQueueHandle; }

	void              deinit();

	void              flush(); // flushes the command list so that no work is being done

	// Command Lists
	GpuCommandList*   getCommandList(GpuCommandListType Type = GpuCommandListType::Graphics);
	u64               executeCommandLists(farray<GpuCommandList*> CommandLists);
	void              processCommandLists(); // Call once per frame, checks if in-flight commands lists are completed
    void              submitEmptyCommandList(GpuCommandList* CommandList);


	// Synchronization
	u64               signal();                                  // (nonblocking) signals the fence and returns its current value
	bool              isFenceComplete(u64 FenceValue);           // (nonblocking) check the currnt value of a fence
	GpuFenceResult    waitForFence(u64 FenceValue);              // (blocking)    wait for a fence to have passed value
	void              wait(const GpuCommandQueue* OtherQueue);   // (blocking)    wait for another command queue to finish executing

private:
	GpuDevice*               mDevice       = nullptr;
	GpuCommandQueueType      mType         = GpuCommandQueueType::None;

	ID3D12CommandQueue*       mQueueHandle   = nullptr;

	// Synchronization
	ID3D12Fence*              mQueueFence   = nullptr;
	u64                       mFenceValue   = 0;

	// TODO:
	// - Determine if creating command lists on the fly is cheap
	// - Determine if command lists can be used after submitting
	// - Determine Resource ownsership. Previously command lists were used as sudo-GC.
	struct InFlightList
	{
		GpuCommandList* CmdList;
		u64             FenceValue;
	};

	std::vector<InFlightList>   mInFlightCommandLists                                        = {};
    std::deque<GpuCommandList*> mAvailableFlightCommandLists[u32(GpuCommandListType::Count)] = {}; // slot for each array type

	//std::deque<InFlightList>    mInFlightCommandLists = {};
	//std::deque<GpuCommandList*> mAvailableFlightCommandLists[u32(GpuCommandListType::Count)] = {};

};