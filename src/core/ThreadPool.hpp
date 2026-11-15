#pragma once
#include "core/TaskQueue.hpp"
#include <thread>
#include <vector>
#include <atomic>

namespace mc {

class ThreadPool {
public:
    explicit ThreadPool(int numThreads);
    ~ThreadPool();

    void enqueue(int priority, TaskQueue::Task task);
    void stop();

    int threadCount() const noexcept { return (int)m_workers.size(); }

private:
    void workerLoop();

    TaskQueue m_tasks;
    std::vector<std::thread> m_workers;
    std::atomic<bool> m_running{true};
};

} // namespace mc