#include "ThreadPool.hpp"

ThreadPool::ThreadPool(size_t numThreads) {
    if (numThreads == 0) {
        numThreads = 1;
    }
    workers.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this] { workerLoop(); });
    }
}

ThreadPool::~ThreadPool() {
    stop = true;
    condition.notify_all();
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (stop) {
            return;
        }
        tasks.push(std::move(task));
    }
    condition.notify_one();
}

size_t ThreadPool::activeThreads() const {
    return busyCount.load();
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] { return stop || !tasks.empty(); });
            if (stop && tasks.empty()) {
                return;
            }
            task = std::move(tasks.front());
            tasks.pop();
            ++busyCount;
        }

        task();
        --busyCount;
    }
}
