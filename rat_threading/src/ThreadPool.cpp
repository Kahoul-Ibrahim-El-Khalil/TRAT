#include "rat/ThreadPool.hpp"

#ifdef DEBUG_RAT_THREADING
    #include <spdlog/spdlog.h>
    #include <spdlog/sinks/stdout_color_sinks.h>
    struct _rat_logging_initializer {
        _rat_logging_initializer() {
            spdlog::set_level(spdlog::level::debug); // Show debug and above
            spdlog::set_pattern("[%H:%M:%S] [%^%l%$] %v"); // Optional: pretty format
        }
    } _rat_logging_initializer_instance;

    #define DEBUG_LOG(...) spdlog::debug(__VA_ARGS__)
    #define ERROR_LOG(...) spdlog::error(__VA_ARGS__)
#else
    #define DEBUG_LOG(...) do { } while(0)
    #define ERROR_LOG(...) do { } while(0)
#endif

namespace rat::threading {

ThreadPool::ThreadPool(uint8_t Num_Threads, uint8_t Max_Queue_Length) 
    : max_queue_length(Max_Queue_Length) {
    this->start(Num_Threads);
}

ThreadPool::~ThreadPool() {
    this->stop();
}
uint8_t ThreadPool::getWorkersSize() {
    return this->workers.size();
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
    DEBUG_LOG("Starting Thread Pool of {} size", Num_Threads);
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
                    
                    if (stop_flag && tasks.empty())   return;
                    
                    if (!tasks.empty()) {
                        task = std::move(tasks.front()); // assign to local variable
                        tasks.pop();
                    } else continue;
                }
                DEBUG_LOG("Pushing the task into the worker thread"); 
                if (task) task();
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
            DEBUG_LOG("Joining the worker thread");
            worker.join();
        }
    }
    DEBUG_LOG("All worker threads joined"); 
    workers.clear();
}

} // namespace rat::threading

#undef DEBUG_LOG
#undef ERROR_LOG
#ifdef DEBUG_RAT_THREADING
    #undef DEBUG_RAT_THREADING
#endif
