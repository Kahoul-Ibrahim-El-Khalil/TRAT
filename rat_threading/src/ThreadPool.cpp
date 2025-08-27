#include "rat/ThreadPool.hpp"
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <stdexcept>

namespace rat {

ThreadPool::ThreadPool(size_t Num_Threads) {
    this->start(Num_Threads);
}

ThreadPool::~ThreadPool() {
    this->stop();
}

void ThreadPool::dropUnfinished() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    std::queue<std::function<void()>> empty;
    std::swap(tasks, empty); // clear task queue
}

void ThreadPool::start(size_t Num_Threads) {
    for (size_t i = 0; i < Num_Threads; ++i) {
        workers.emplace_back([this] {
            for (;;) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    condVar.wait(lock, [this] { return stopFlag || !tasks.empty(); });

                    if (stopFlag && tasks.empty())
                        return; // exit thread

                    if (!tasks.empty()) {
                        task = std::move(tasks.front());
                        tasks.pop();
                    } else {
                        continue;
                    }
                }

                if (task) task();
            }
        });
    }
}

void ThreadPool::stop() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        stopFlag = true;
    }
    condVar.notify_all();

    for (std::thread &worker : workers) {
        if (worker.joinable())
            worker.join();
    }
}

} // namespace rat
