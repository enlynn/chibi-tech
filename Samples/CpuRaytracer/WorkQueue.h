#pragma once

#include <thread>
#include <deque>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>
#include <condition_variable>

#include <Types.h>
#include "Raytracer.h"

class WorkQueue {
public:
    using TaskFunc = std::function<void(RaytracerWork)>;
    struct Task {
        RaytracerWork mRaytracer;
        TaskFunc      mTaskFunction;

        Task(RaytracerWork& tRaytracer, TaskFunc& tTaskFunction) : mRaytracer(tRaytracer), mTaskFunction(std::move(tTaskFunction)) {}
    };

    explicit WorkQueue(int tNumThreads);
    void release();

    static int getSystemThreadCount();
    static void threadExecuteWork(WorkQueue* tWorkQueue);

    void addTask(RaytracerWork& tWork, TaskFunc& tTaskFunction, bool tSignalImmediately = false);

    Task threadWaitAndAcquireWork();

    bool isRunning() { return mIsRunning.load(); }
    size_t taskCount();
    size_t taskCountUnsafe() const {  return mTaskQueue.size(); }
    void clearWorkQueue();

    void signalThreads();
    void waitForWorkToComplete();

    std::atomic<bool>         mIsRunning{false};
    std::condition_variable   mTaskCV{};
    std::mutex                mTaskLock{};
    std::deque<Task>          mTaskQueue{};
    std::vector<std::jthread> mThreadList{};

    // For the main thread to for current work to be completed
    std::atomic<size_t>       mTaskCount{0};
    std::condition_variable   mTaskCountCV{};
};