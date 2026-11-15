#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace mc {

class TaskQueue {
public:
    using Task = std::function<void()>;

    struct PrioritizedTask {
        int priority;
        Task task;
        bool operator<(const PrioritizedTask& other) const {
            return priority > other.priority;
        }
    };

    void push(int priority, Task task) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push({priority, std::move(task)});
        }
        m_cv.notify_one();
    }

    bool tryPop(Task& task) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) return false;
        task = std::move(m_queue.top().task);
        m_queue.pop();
        return true;
    }

    void waitAndPop(Task& task) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return !m_queue.empty() || m_stop; });
        if (m_stop) return;
        task = std::move(m_queue.top().task);
        m_queue.pop();
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stop = true;
        }
        m_cv.notify_all();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

private:
    std::priority_queue<PrioritizedTask> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stop = false;
};

} // namespace mc