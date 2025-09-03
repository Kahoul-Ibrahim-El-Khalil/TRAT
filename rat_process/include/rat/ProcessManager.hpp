#pragma once
#include "rat/ThreadPool.hpp"
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#ifdef _WIN32
#include <windows.h>

#else
#include <sys/types.h>
#endif

namespace rat::process {

class ProcessManager {
  private:
	std::string command;
	std::function<void(int64_t)> creation_callback;

	int64_t process_id = 0;
	int exit_code = -1;
	bool timed_out = false;

	std::string stdout_str;
	std::string stderr_str;

	std::mutex stdout_mutex;
	std::mutex stderr_mutex;

	std::chrono::milliseconds timeout = std::chrono::milliseconds(0);

	bool executeInternal();

  public:
	ProcessManager() = default;
	~ProcessManager();

	ProcessManager(const ProcessManager &) = delete;
	ProcessManager &operator=(const ProcessManager &) = delete;
	ProcessManager(ProcessManager &&) = default;
	ProcessManager &operator=(ProcessManager &&) = default;

	// Getters
	int64_t getProcessId() const {
		return process_id;
	}

	int getExitCode() const {
		return exit_code;
	}

	const std::string &getStdout() const {
		return stdout_str;
	}
	const std::string &getStderr() const {
		return stderr_str;
	}
	bool wasTimedOut() const {
		return timed_out;
	}
	bool wasSuccessful() const {
		return exit_code == 0 && !timed_out;
	}

	void setCommand(const std::string &arg_Command);
	void setCommand(std::string &&arg_Command);
	void setTimeout(const std::chrono::milliseconds &arg_Timeout);

	void setProcessCreationCallback(std::function<void(int64_t)> &&Creation_Callback);

	void execute();
};

} // namespace rat::process
