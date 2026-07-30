#include "ThreadPool.hpp"

ThreadPool::ThreadPool(size_t /*numThreads*/) {}

ThreadPool::~ThreadPool() = default;

void ThreadPool::enqueue(std::function<void()> /*task*/) {}

size_t ThreadPool::activeThreads() const { return 0; }

void ThreadPool::workerLoop() {}
