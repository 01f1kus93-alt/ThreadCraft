#include "core/ThreadPool.hpp"
#include <raylib.h>

namespace mc {

ThreadPool::ThreadPool(int numThreads) {
    for (int i = 0; i < numThreads; ++i) {
        m_workers.emplace_back(&ThreadPool::workerLoop, this);
    }
    TraceLog(LOG_INFO, "ThreadPool: %d workers started", numThreads);
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::enqueue(int priority, TaskQueue::Task task) {
    if (!m_running) return;
    m_tasks.push(priority, std::move(task));
}

void ThreadPool::workerLoop() {
    while (m_running) {
        TaskQueue::Task task;
        m_tasks.waitAndPop(task);
        if (!m_running) break;
        if (task) task();
    }
}

void ThreadPool::stop() {
    if (!m_running.exchange(false)) return;
    m_tasks.stop();
    for (auto& t : m_workers) {
        if (t.joinable()) t.join();
    }
    m_workers.clear();
    TraceLog(LOG_INFO, "ThreadPool stopped");
}

} // namespace mc