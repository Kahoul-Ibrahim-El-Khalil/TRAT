#pragma once
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace rat::threading {

class ThreadPool {
  public:
	explicit ThreadPool(uint8_t Num_Threads = 1, uint8_t Max_Queue_Length = 2);
	~ThreadPool();

	void dropUnfinished();
	void start(uint8_t Num_Threads);
	void stop();

	template <class T> auto enqueue(T task) -> std::future<decltype(task())> {
		using return_type = decltype(task());
		auto wrapper = std::make_shared<std::packaged_task<return_type()>>(std::move(task));
		std::future<return_type> fut = wrapper->get_future();

		{
			std::lock_guard<std::mutex> lock(queue_mutex);
			if (!stop_flag && tasks.size() < this->max_queue_length) {
				tasks.emplace([wrapper] { (*wrapper)(); });
				cond_var.notify_one();
			}
		}

		return fut;
	}

	uint8_t getPendingWorkersCount();
	uint8_t getWorkersSize();

  private:
	std::vector<std::thread> workers;
	std::queue<std::function<void()>> tasks;
	uint8_t max_queue_length;
	std::mutex queue_mutex;
	std::condition_variable cond_var;
	bool stop_flag{false};
	bool started{false};
};

} // namespace rat::threading
