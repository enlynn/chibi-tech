#include "stdafx.h"
#include "D3D12Common.h"
#include "GpuQueueManager.h"

GpuQueueManager::GpuQueueManager(GpuDevice* tDevice)
	: mGraphicsQueue(GpuQueueType::Graphics, tDevice)
	, mComputeQueue(GpuQueueType::Compute,   tDevice)
	, mCopyQueue(GpuQueueType::Copy,         tDevice)
{
}

void GpuQueueManager::destroy()
{
	mGraphicsQueue.deinit();
	mComputeQueue.deinit();
	mCopyQueue.deinit();
}

void GpuQueueManager::processPendingCommandLists()
{
	mGraphicsQueue.processCommandLists();
	mComputeQueue.processCommandLists();
	mCopyQueue.processCommandLists();
}

GpuQueue* GpuQueueManager::getQueue(GpuQueueType tQueue)
{
	ASSERT(tQueue != GpuQueueType::None);

	switch (tQueue) {
	case GpuQueueType::Graphics: return &mGraphicsQueue;
	case GpuQueueType::Compute:  return &mComputeQueue;
	case GpuQueueType::Copy:     return &mCopyQueue;
	}

	return nullptr;
}

void GpuQueueManager::flush(GpuQueueType tType)
{
	auto queue = getQueue(tType);
	queue->flush();
}

void GpuQueueManager::flushGpu()
{
	flush(GpuQueueType::Graphics);
	flush(GpuQueueType::Compute);
	flush(GpuQueueType::Copy);
}

GpuFence GpuQueueManager::signal(GpuFence tFence)
{
	auto queue = getQueue(GpuQueueType(tFence.mType));
	return queue->signal();
}

bool GpuQueueManager::isFenceComplete(GpuFence tFenceValue)
{
	auto queue = getQueue(GpuQueueType(tFenceValue.mType));
	return queue->isFenceComplete(tFenceValue);
}

bool GpuQueueManager::waitForFence(GpuFence tFenceValue)
{
	auto queue = getQueue(GpuQueueType(tFenceValue.mType));
	return queue->waitForFence(tFenceValue);
}

void GpuQueueManager::wait(GpuQueueType tSrcQueue, GpuQueueType tQueueToWaitOn)
{
	auto waitingQueue  = getQueue(tSrcQueue);
	auto queueToWaitOn = getQueue(tQueueToWaitOn);
	waitingQueue->wait(queueToWaitOn);
}

GpuCommandListUPtr GpuQueueManager::getGraphicsCommandList()
{
	return mGraphicsQueue.getCommandList(GpuCommandListType::Graphics);
}

GpuCommandListUPtr GpuQueueManager::getComputeCommandList(bool tUseGraphicsQueue)
{
	auto queue = tUseGraphicsQueue ? &mGraphicsQueue : &mComputeQueue;
	return queue->getCommandList(GpuCommandListType::Compute);
}

GpuCommandListUPtr GpuQueueManager::getCopyCommandList(bool tUseGraphicsQueue)
{
	auto queue = tUseGraphicsQueue ? &mGraphicsQueue : &mCopyQueue;
	return queue->getCommandList(GpuCommandListType::Copy);
}

GpuFence GpuQueueManager::submitCommandList(GpuCommandListUPtr tCommandList, GpuQueueType tQueue)
{
	if (tQueue == GpuQueueType::None)
	{ // determine the queue type based on the command list
		auto listType = tCommandList->getType();
		switch (listType)
		{
		case GpuCommandListType::Graphics: tQueue = GpuQueueType::Graphics; break;
		case GpuCommandListType::Compute:  tQueue = GpuQueueType::Compute;  break;
		case GpuCommandListType::Copy:     tQueue = GpuQueueType::Copy;     break;
		case GpuCommandListType::Indirect: tQueue = GpuQueueType::Graphics; break;
		case GpuCommandListType::Count:    [[fallthrough]];
		case GpuCommandListType::None:     [[fallthrough]];
		default:                           ASSERT(false); break; //invalid command list types
		}
	}

	auto queue = getQueue(tQueue);
	return queue->executeCommandLists(std::span(&tCommandList, 1));
}

void GpuQueueManager::submitEmptyCommandList(GpuCommandListUPtr tCommandList, GpuQueueType tQueue)
{
	if (tQueue == GpuQueueType::None)
	{ // determine the queue type based on the command list
		auto listType = tCommandList->getType();
		switch (listType)
		{
		case GpuCommandListType::Graphics: tQueue = GpuQueueType::Graphics; break;
		case GpuCommandListType::Compute:  tQueue = GpuQueueType::Compute;  break;
		case GpuCommandListType::Copy:     tQueue = GpuQueueType::Copy;     break;
		case GpuCommandListType::Indirect: tQueue = GpuQueueType::Graphics; break;
		case GpuCommandListType::Count: [[fallthrough]];
		case GpuCommandListType::None: [[fallthrough]];
		default:                           ASSERT(false); break; //invalid command list types
		}
	}

	auto queue = getQueue(tQueue);
	queue->submitEmptyCommandList(std::move(tCommandList));
}

GpuFence GpuQueueManager::submitCommandLists(std::span<GpuCommandListUPtr> tCommandLists, GpuQueueType tQueue)
{
	ASSERT(tCommandLists.size() > 0);

	if (tQueue == GpuQueueType::None)
	{ // determine the queue type based on the command list
		auto listType = tCommandLists[0]->getType();
		switch (listType)
		{
		case GpuCommandListType::Graphics: tQueue = GpuQueueType::Graphics; break;
		case GpuCommandListType::Compute:  tQueue = GpuQueueType::Compute;  break;
		case GpuCommandListType::Copy:     tQueue = GpuQueueType::Copy;     break;
		case GpuCommandListType::Indirect: tQueue = GpuQueueType::Graphics; break;
		case GpuCommandListType::Count:    [[fallthrough]];
		case GpuCommandListType::None:     [[fallthrough]];
		default:                           ASSERT(false); break; //invalid command list types
		}
	}

	auto queue = getQueue(tQueue);
	return queue->executeCommandLists(tCommandLists);
}
