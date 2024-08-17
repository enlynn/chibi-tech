#pragma once

#include <memory>

#include "GpuQueue.h"

class GpuQueueManager {
public:
	GpuQueueManager(std::shared_ptr<GpuDevice> tDevice);
	~GpuQueueManager() { destroy(); }

	void destroy();

	void processPendingCommandLists();

	GpuQueue* getQueue(GpuQueueType tQueue = GpuQueueType::Graphics);

	// Flush a specific command queue
	void flush(GpuQueueType tType);
	// Flush all command queues 
	void flushGpu();

	// (nonblocking) signals the fence and returns its current value
	GpuFence signal(GpuFence tFence);
	GpuFence signalGraphicsQueue() { return mGraphicsQueue.signal(); }
	GpuFence signalCopyQueue()     { return mCopyQueue.signal();     }
	GpuFence signalComputeQueue()  { return mComputeQueue.signal();  }
	
	// (nonblocking) check the currnt value of a fence
	bool isFenceComplete(GpuFence tFenceValue);    
	// (blocking) wait for a fence to have passed value
	bool waitForFence(GpuFence tFenceValue);
	// (blocking) wait for another command queue to finish executing
	void wait(GpuQueueType tSrcQueue, GpuQueueType tQueueToWaitOn);

	// Compute and Copy operations are allowed on the graphics queue. However, only compute
	// and copy work can execute on their respective queues.
	GpuCommandListUPtr getGraphicsCommandList();
	GpuCommandListUPtr getComputeCommandList(bool tUseGraphicsQueue = false);
	GpuCommandListUPtr getCopyCommandList(bool tUseGraphicsQueue = false);

	// @assume all command lists are from the same queue.
	// If queue type is None, then the queue will be chosen based on the command list type.
	GpuFence submitCommandList(GpuCommandListUPtr tCommandList, GpuQueueType tQueue = GpuQueueType::None);
	void     submitEmptyCommandList(GpuCommandListUPtr tCommandList, GpuQueueType tQueue = GpuQueueType::None);
	GpuFence submitCommandLists(std::span<GpuCommandListUPtr> tCommandLists, GpuQueueType tQueue = GpuQueueType::None);

private:
	GpuQueue mGraphicsQueue;
	GpuQueue mComputeQueue;
	GpuQueue mCopyQueue;
};