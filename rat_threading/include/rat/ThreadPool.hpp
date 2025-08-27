#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>

namespace rat {

class ThreadPool {
public:
    explicit ThreadPool(size_t Num_Threads);
    ~ThreadPool();


    void dropUnfinished();
    void start(size_t Num_Threads);
    void stop();
template <class T>
auto enqueue(T task) -> std::future<decltype(task())> {
    using return_type = decltype(task());

    auto wrapper = std::make_shared<std::packaged_task<return_type()>>(std::move(task));

    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        if (stopFlag) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        tasks.emplace([=] { (*wrapper)(); });
    }

    condVar.notify_one();
    return wrapper->get_future();
}


private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condVar;
    bool stopFlag{false};
};

} // namespace rat

