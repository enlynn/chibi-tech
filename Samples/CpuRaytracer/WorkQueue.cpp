#include "WorkQueue.h"


WorkQueue::WorkQueue(const int tNumThreads) {
    mIsRunning.store(true);

    const int maxThreads = getSystemThreadCount();
    int threadCount = (tNumThreads > maxThreads) ? maxThreads : tNumThreads;

    mThreadList.reserve(threadCount);
    for (int i = 0; i < threadCount; ++i) {
        mThreadList.emplace_back(threadExecuteWork, this);
    }
}

void WorkQueue::threadExecuteWork(WorkQueue* tWorkQueue) {
    do {
        auto task = tWorkQueue->threadWaitAndAcquireWork();
        task.mTaskFunction(task.mRaytracer);

        tWorkQueue->mTaskCount -= 1;
        tWorkQueue->mTaskCountCV.notify_one();

        // get next work
    } while (tWorkQueue->isRunning());
}

size_t WorkQueue::taskCount() {
    std::lock_guard guard(mTaskLock);

    size_t size = mTaskQueue.size();
    return size;
}

WorkQueue::Task WorkQueue::threadWaitAndAcquireWork() {
    std::unique_lock lk(mTaskLock);

    if (taskCountUnsafe() == 0) {
        mTaskCV.wait(lk, [this] { return taskCountUnsafe() > 0; });
    }

    Task result = mTaskQueue[0];
    mTaskQueue.pop_front();

    return result;
}

void WorkQueue::release() {
    mIsRunning.store(false);

    for (auto& thread : mThreadList) {
        thread.join();
    }
}

int WorkQueue::getSystemThreadCount() {
    return (int)std::thread::hardware_concurrency();
}

void WorkQueue::addTask(RaytracerWork &tWork, TaskFunc& tTaskFunction, bool tSignalImmediately) {
    std::lock_guard guard(mTaskLock);

    mTaskQueue.emplace_back(tWork, tTaskFunction);
    mTaskCount += 1;

    if (tSignalImmediately) {
        mTaskCV.notify_all();
    }
}

void WorkQueue::clearWorkQueue() {
    std::lock_guard guard(mTaskLock);
    mTaskQueue.clear();
}

void WorkQueue::signalThreads() {
    std::lock_guard guard(mTaskLock);
    mTaskCV.notify_all();
}

void WorkQueue::waitForWorkToComplete() {
    std::unique_lock lk(mTaskLock);
    if (mTaskCount.load() > 0) {
        mTaskCountCV.wait(lk, [this] { return mTaskCount.load() == 0; });
    }
}