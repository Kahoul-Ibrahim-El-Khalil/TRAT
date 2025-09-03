#include "rat/ProcessManager.hpp"
#include <chrono>
#include <future>
#include <memory>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#ifdef DEBUG_RAT_PROCESS
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

// Make sure we provide function names to spdlog
#ifndef SPDLOG_FUNCTION
#if defined(__PRETTY_FUNCTION__)
#define SPDLOG_FUNCTION __PRETTY_FUNCTION__
#else
#define SPDLOG_FUNCTION __func__
#endif
#endif

static void _rat_process_init_logging() {
	static bool is_process_logging_initialized = false;
	if (!is_process_logging_initialized) {
		// Show time, level, module, file:line function, and the message
		spdlog::set_level(spdlog::level::debug);
		spdlog::set_pattern("[%H:%M:%S] [%^%l%$] [rat_process] [%s:%# %!] %v");
		is_process_logging_initialized = true;
	}
}

// Use source_loc so pattern can render file/line/function
#define PROCESS_DEBUG_LOG(fmt, ...)                                                                                                                  \
	do {                                                                                                                                             \
		_rat_process_init_logging();                                                                                                                 \
		spdlog::log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::debug, fmt, ##__VA_ARGS__);                              \
	} while (0)

#define PROCESS_ERROR_LOG(fmt, ...)                                                                                                                  \
	do {                                                                                                                                             \
		_rat_process_init_logging();                                                                                                                 \
		spdlog::log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::err, fmt, ##__VA_ARGS__);                                \
	} while (0)
#else
#define PROCESS_DEBUG_LOG(...)                                                                                                                       \
	do {                                                                                                                                             \
	} while (0)
#define PROCESS_ERROR_LOG(...)                                                                                                                       \
	do {                                                                                                                                             \
	} while (0)
#endif

namespace rat::process {

constexpr size_t PROCESS_BUFFER_SIZE = 512;

ProcessManager::~ProcessManager() {}

void ProcessManager::setCommand(const std::string &arg_Command) {
	this->command = arg_Command;
}

void ProcessManager::setCommand(std::string &&arg_Command) {
	this->command = std::move(arg_Command);
}

void ProcessManager::setTimeout(const std::chrono::milliseconds &arg_Timeout) {
	this->timeout = arg_Timeout;
}

void ProcessManager::setProcessCreationCallback(std::function<void(int64_t)> &&Creation_Callback) {
	this->creation_callback = std::move(Creation_Callback);
}

#ifdef _WIN32
bool ProcessManager::executeInternal() {
	PROCESS_DEBUG_LOG("Preparing to run process on Windows: '{}'", command);

	SECURITY_ATTRIBUTES security_attribute{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
	HANDLE handle_for_reading_output = nullptr;
	HANDLE handle_for_writing_output = nullptr;
	HANDLE handle_for_reading_error = nullptr;
	HANDLE handle_for_writing_error = nullptr;

	if (!CreatePipe(&handle_for_reading_output, &handle_for_writing_output, &security_attribute, 0) ||
		!CreatePipe(&handle_for_reading_error, &handle_for_writing_error, &security_attribute, 0)) {
		PROCESS_ERROR_LOG("Failed to create pipes for process '{}'", command);
		return false;
	}

	SetHandleInformation(handle_for_reading_output, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(handle_for_reading_error, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOA si{};
	si.cb = sizeof(si);
	si.dwFlags |= STARTF_USESTDHANDLES;
	si.hStdOutput = handle_for_writing_output;
	si.hStdError = handle_for_writing_error;
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

	PROCESS_INFORMATION pi{};
	BOOL ok = CreateProcessA(nullptr, const_cast<LPSTR>(command.c_str()), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi);

	if (!ok) {
		PROCESS_ERROR_LOG("Failed to create process: '{}', error code {}", command, GetLastError());
		CloseHandle(handle_for_reading_output);
		CloseHandle(handle_for_writing_output);
		CloseHandle(handle_for_reading_error);
		CloseHandle(handle_for_writing_error);
		return false;
	}

	this->process_id = static_cast<int64_t>(pi.dwProcessId);
	PROCESS_DEBUG_LOG("Process '{}' created successfully (PID={})", command, this->process_id);

	if (this->creation_callback) {
		try {
			this->creation_callback(this->process_id);
		} catch (...) {
			PROCESS_ERROR_LOG("Creation callback threw an exception for process '{}'", command);
		}
	}

	CloseHandle(handle_for_writing_output);
	CloseHandle(handle_for_writing_error);

	auto stdout_ptr = std::make_shared<std::string>();
	auto stderr_ptr = std::make_shared<std::string>();

	auto running = std::make_shared<bool>(true);

	std::thread thread_for_handling_process_output, thread_for_handling_process_error;
	std::future<void> future_out, future_err;

	thread_for_handling_process_output = std::thread([command_copy = this->command, stdout_ptr, running, handle_for_reading_output, this] {
		PROCESS_DEBUG_LOG("Started output reader thread for '{}'", command_copy);
		char buf[PROCESS_BUFFER_SIZE];
		DWORD n = 0;
		while (*running) {
			if (!ReadFile(handle_for_reading_output, buf, DWORD(sizeof(buf)), &n, nullptr) || n == 0)
				break;
			std::unique_lock<std::mutex> stdout_lock(this->stdout_mutex);
			stdout_ptr->append(buf, buf + n);
		}
		PROCESS_DEBUG_LOG("Output reader thread finished for '{}'", command_copy);
	});

	thread_for_handling_process_error = std::thread([command_copy = this->command, stderr_ptr, running, handle_for_reading_error, this] {
		PROCESS_DEBUG_LOG("Started error reader thread for '{}'", command_copy);
		char buf[PROCESS_BUFFER_SIZE];
		DWORD n = 0;
		while (*running) {
			if (!ReadFile(handle_for_reading_error, buf, DWORD(sizeof(buf)), &n, nullptr) || n == 0)
				break;
			std::unique_lock<std::mutex> stderr_lock(this->stderr_mutex);
			stderr_ptr->append(buf, buf + n);
		}
		PROCESS_DEBUG_LOG("Error reader thread finished for '{}'", command_copy);
	});

	DWORD waitTime = (timeout.count() > 0) ? DWORD(timeout.count()) : INFINITE;
	PROCESS_DEBUG_LOG("Waiting for process '{}' with timeout {} ms", command, timeout.count());
	DWORD result = WaitForSingleObject(pi.hProcess, waitTime);

	if (result == WAIT_TIMEOUT) {
		PROCESS_ERROR_LOG("Process '{}' timed out, terminating...", command);
		TerminateProcess(pi.hProcess, 1);
		WaitForSingleObject(pi.hProcess, INFINITE);
		PROCESS_DEBUG_LOG("Process '{}' terminated due to timeout", command);
		timed_out = true;
	} else {
		PROCESS_DEBUG_LOG("Process '{}' completed normally", command);
		timed_out = false;
	}

	// FIXED: Set running to false FIRST
	*running = false;

	// FIXED: Cancel pending I/O to wake up reader threads
	CancelIo(handle_for_reading_output);
	CancelIo(handle_for_reading_error);

	if (thread_for_handling_process_output.joinable())
		thread_for_handling_process_output.join();
	if (thread_for_handling_process_error.joinable())
		thread_for_handling_process_error.join();
	CloseHandle(handle_for_reading_output);
	CloseHandle(handle_for_reading_error);

	DWORD exitCode = DWORD(-1);
	GetExitCodeProcess(pi.hProcess, &exitCode);
	PROCESS_DEBUG_LOG("Process '{}' exited with code {}", command, exitCode);

	exit_code = static_cast<int>(exitCode);
	stdout_str = std::move(*stdout_ptr);
	stderr_str = std::move(*stderr_ptr);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return !timed_out;
}
#else
bool ProcessManager::executeInternal() {
	PROCESS_DEBUG_LOG("Preparing to run process on POSIX: '{}' (timeout={} ms)", command, timeout.count());

	int pipe_out[2]{-1, -1}, pipe_err[2]{-1, -1};

	if (pipe(pipe_out) == -1) {
		PROCESS_ERROR_LOG("pipe(pipe_out) failed: errno={} ({})", errno, strerror(errno));
		return false;
	}
	if (pipe(pipe_err) == -1) {
		PROCESS_ERROR_LOG("pipe(pipe_err) failed: errno={} ({})", errno, strerror(errno));
		close(pipe_out[0]);
		close(pipe_out[1]);
		return false;
	}

	PROCESS_DEBUG_LOG("Created pipes: out[r={}, w={}] err[r={}, w={}]", pipe_out[0], pipe_out[1], pipe_err[0], pipe_err[1]);

	pid_t pid = fork();
	if (pid < 0) {
		PROCESS_ERROR_LOG("fork() failed: errno={} ({})", errno, strerror(errno));
		close(pipe_out[0]);
		close(pipe_out[1]);
		close(pipe_err[0]);
		close(pipe_err[1]);
		return false;
	}

	if (pid == 0) {
		// Child
		(void)setsid(); // best-effort; ignore error

		if (dup2(pipe_out[1], STDOUT_FILENO) == -1) {
			// If dup2 fails, at least exit with a distinct code:
			_exit(127);
		}
		if (dup2(pipe_err[1], STDERR_FILENO) == -1) {
			_exit(127);
		}

		// Close all fds we no longer need in the child
		close(pipe_out[0]);
		close(pipe_out[1]);
		close(pipe_err[0]);
		close(pipe_err[1]);

		// Use /bin/sh -c "command"
		execl("/bin/sh", "sh", "-c", command.c_str(), (char *)nullptr);
		// If we got here execl failed:
		_exit(127);
	}

	// Parent
	this->process_id = static_cast<int64_t>(pid);
	PROCESS_DEBUG_LOG("Forked process OK: PID={} cmd='{}'", this->process_id, command);

	if (this->creation_callback) {
		try {
			creation_callback(this->process_id);
		} catch (...) {
			PROCESS_ERROR_LOG("Creation callback threw");
		}
	}

	// Close write ends in parent
	close(pipe_out[1]);
	close(pipe_err[1]);
	PROCESS_DEBUG_LOG("Parent closed write ends: out[w closed], err[w closed]");

	auto stdout_ptr = std::make_shared<std::string>();
	auto stderr_ptr = std::make_shared<std::string>();
	auto running = std::make_shared<bool>(true);

	std::thread t_out, t_err;

	auto reader_stdout = [this, stdout_ptr, running, rfd = pipe_out[0], cmd = command]() {
		PROCESS_DEBUG_LOG("STDOUT reader start (fd={})", rfd);
		char buf[PROCESS_BUFFER_SIZE];
		while (*running) {
			ssize_t n = ::read(rfd, buf, sizeof(buf));
			if (n > 0) {
				PROCESS_DEBUG_LOG("STDOUT read {} bytes", n); // NEW: byte trace
				std::unique_lock<std::mutex> g(this->stdout_mutex);
				stdout_ptr->append(buf, size_t(n));
			} else if (n == 0) {
				PROCESS_DEBUG_LOG("STDOUT EOF");
				break;
			} else { // n < 0
				if (errno == EINTR) {
					PROCESS_DEBUG_LOG("STDOUT read interrupted (EINTR), retrying...");
					continue;
				}
				PROCESS_ERROR_LOG("STDOUT read error: errno={} ({})", errno, strerror(errno));
				break;
			}
		}
		// Close our read fd at the end of the loop
		if (rfd >= 0) {
			close(rfd);
			PROCESS_DEBUG_LOG("STDOUT reader closed fd");
		}
		PROCESS_DEBUG_LOG("STDOUT reader end");
	};

	auto reader_stderr = [this, stderr_ptr, running, rfd = pipe_err[0], cmd = command]() {
		PROCESS_DEBUG_LOG("STDERR reader start (fd={})", rfd);
		char buf[PROCESS_BUFFER_SIZE];
		while (*running) {
			ssize_t n = ::read(rfd, buf, sizeof(buf));
			if (n > 0) {
				PROCESS_DEBUG_LOG("STDERR read {} bytes", n); // NEW: byte trace
				std::unique_lock<std::mutex> g(this->stderr_mutex);
				stderr_ptr->append(buf, size_t(n));
			} else if (n == 0) {
				PROCESS_DEBUG_LOG("STDERR EOF");
				break;
			} else {
				if (errno == EINTR) {
					PROCESS_DEBUG_LOG("STDERR read interrupted (EINTR), retrying...");
					continue;
				}
				PROCESS_ERROR_LOG("STDERR read error: errno={} ({})", errno, strerror(errno));
				break;
			}
		}
		if (rfd >= 0) {
			close(rfd);
			PROCESS_DEBUG_LOG("STDERR reader closed fd");
		}
		PROCESS_DEBUG_LOG("STDERR reader end");
	};

	t_out = std::thread(reader_stdout);
	t_err = std::thread(reader_stderr);
	int status = 0;
	timed_out = false;

	if (timeout.count() > 0) {
		auto deadline = std::chrono::steady_clock::now() + timeout;
		PROCESS_DEBUG_LOG("Waiting for PID={} with timeout={} ms", pid, timeout.count());
		while (true) {
			pid_t ret = waitpid(pid, &status, WNOHANG);
			if (ret == pid) {
				PROCESS_DEBUG_LOG("waitpid: child exited");
				break;
			}
			if (ret == -1) {
				PROCESS_ERROR_LOG("waitpid failed: errno={} ({})", errno, strerror(errno));
				break;
			}
			if (std::chrono::steady_clock::now() >= deadline) {
				PROCESS_ERROR_LOG("Timeout: killing process group -{}", pid);
				kill(-pid, SIGKILL);
				timed_out = true;
				(void)waitpid(pid, &status, 0);
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	} else {
		PROCESS_DEBUG_LOG("Waiting indefinitely for PID={}", pid);
		(void)waitpid(pid, &status, 0);
	}

	// Tell readers to stop and ensure reads unblock (their close happens in readers)
	*running = false;
	PROCESS_DEBUG_LOG("Set running=false; joining reader(s)");

	if (t_out.joinable())
		t_out.join();
	if (t_err.joinable())
		t_err.join();

	int exit_code_value = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
	PROCESS_DEBUG_LOG("PID={} exit_code={}", pid, exit_code_value);

	exit_code = exit_code_value;
	stdout_str = std::move(*stdout_ptr);
	stderr_str = std::move(*stderr_ptr);

	PROCESS_DEBUG_LOG("Captured STDOUT ({} bytes), STDERR ({} bytes)", stdout_str.size(), stderr_str.size());

	return !timed_out;
}

#endif

void ProcessManager::execute() {
	// Validate state before execution
	if (command.empty()) {
		PROCESS_ERROR_LOG("Cannot execute: empty command");
		return;
	}
	this->executeInternal();
	PROCESS_DEBUG_LOG("EXIT_CODE: {}\nSTDOUT: {}\nSTDERR: {}", this->exit_code, this->stdout_str, this->stderr_str);
}

} // namespace rat::process
