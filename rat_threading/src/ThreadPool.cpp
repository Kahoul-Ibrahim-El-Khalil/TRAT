#include "rat/ThreadPool.hpp"

namespace rat::threading {

ThreadPool::ThreadPool(uint8_t Num_Threads, uint8_t Max_Queue_Length) 
    : max_queue_length(Max_Queue_Length) {
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

uint8_t ThreadPool::getPendingWorkersCount() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    return static_cast<uint8_t>(tasks.size());
}

void ThreadPool::start(uint8_t Num_Threads) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    
    // Prevent multiple starts
    if (started) {
        return;
    }
    
    stop_flag = false;
    started = true;
    
    for (uint8_t i = 0; i < Num_Threads; ++i) {
        workers.emplace_back([this] {
            for (;;) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    cond_var.wait(lock, [this] { return stop_flag || !tasks.empty(); });
                    
                    if (stop_flag && tasks.empty())
                        return;
                    
                    if (!tasks.empty()) {
                        task = std::move(tasks.front()); // assign to local variable
                        tasks.pop();
                    } else {
                        continue;
                    }
                }
                
                if (task) {
                    task();
                }
            }
        });
    }
}

void ThreadPool::stop() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        if (!started) {
            return;
        }
        stop_flag = true;
        started = false;
    }
    
    cond_var.notify_all();
    
    for (std::thread &worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    // Clear workers vector after joining all threads
    workers.clear();
}

} // namespace rat::threading