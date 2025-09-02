#include "rat/ProcessManager.hpp"
#include <thread>
#include <chrono>
#include <vector>
#include <memory>
#include <future>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#endif

#ifdef DEBUG_RAT_PROCESS
    #include <spdlog/spdlog.h>
    #include <spdlog/sinks/stdout_color_sinks.h>
    inline void _rat_init_logging() {
        static bool initialized = false;
        if (!initialized) {
            spdlog::set_level(spdlog::level::debug);           
            // Add module name "rat_handler" in the prefix
            spdlog::set_pattern("[%H:%M:%S] [%^%l%$] [rat_process] %v"); 
            initialized = true;
        }
    }

    #define DEBUG_LOG(...) do { _rat_init_logging(); spdlog::debug(__VA_ARGS__); } while(0)
    #define ERROR_LOG(...) do { _rat_init_logging(); spdlog::error(__VA_ARGS__); } while(0)

#else
    #define DEBUG_LOG(...) do { } while(0)
    #define ERROR_LOG(...) do { } while(0)
#endif
namespace rat::process {

constexpr size_t PROCESS_BUFFER_SIZE = 512;

ProcessManager::~ProcessManager() {
    // Cleanup if needed
}

void ProcessManager::setCommand(const std::string& arg_Command) {
    this->command = arg_Command;
}

void ProcessManager::setCommand(std::string&& arg_Command) {
    this->command = std::move(arg_Command);
}

void ProcessManager::setSecondaryThreadPool(::rat::threading::ThreadPool* p_Secondary_Thread_Pool) {
    this->p_secondary_threadpool = p_Secondary_Thread_Pool;
}
    
void ProcessManager::setTimeout(std::chrono::milliseconds arg_Timeout) {
    this->timeout = arg_Timeout;
}

void ProcessManager::setProcessCreationCallback(std::function<void(pid_t)>&& Creation_Callback) {
    this->creation_callback = std::move(Creation_Callback);
}

#ifdef _WIN32
bool ProcessManager::executeInternal() {
    DEBUG_LOG("Preparing to run process on Windows: '{}'", command);

    SECURITY_ATTRIBUTES security_attribute{ sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
    HANDLE handle_for_reading_output = nullptr;
    HANDLE handle_for_writing_output = nullptr;
    HANDLE handle_for_reading_error = nullptr;
    HANDLE handle_for_writing_error = nullptr;

    if (!CreatePipe(&handle_for_reading_output, &handle_for_writing_output, &security_attribute, 0) ||
        !CreatePipe(&handle_for_reading_error, &handle_for_writing_error, &security_attribute, 0)) {
        ERROR_LOG("Failed to create pipes for process '{}'", command);
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
    BOOL ok = CreateProcessA(
        nullptr,
        const_cast<LPSTR>(command.c_str()),
        nullptr, nullptr, TRUE,
        0, nullptr, nullptr,
        &si, &pi
    );

    if (!ok) {
        ERROR_LOG("Failed to create process: '{}', error code {}", command, GetLastError());
        CloseHandle(handle_for_reading_output);
        CloseHandle(handle_for_writing_output);
        CloseHandle(handle_for_reading_error);
        CloseHandle(handle_for_writing_error);
        return false;
    }

    // Removed atomic operation
    this->process_id = static_cast<pid_t>(pi.dwProcessId);
    DEBUG_LOG("Process '{}' created successfully (PID={})", command, pi.dwProcessId);

    // Invoke creation callback
    if (this->creation_callback) {
        try {
            this->creation_callback(this->process_id);
        } catch (...) {
            ERROR_LOG("Creation callback threw an exception for process '{}'", command);
        }
    }

    CloseHandle(handle_for_writing_output);
    CloseHandle(handle_for_writing_error);

    auto stdout_ptr = std::make_shared<std::string>();
    auto stderr_ptr = std::make_shared<std::string>();
    // Simplified - just use a regular bool instead of atomic
    auto running = std::make_shared<bool>(true);

    std::thread thread_for_handling_process_output, thread_for_handling_process_error;
    std::future<void> future_out, future_err;
    bool used_pool = (this->p_secondary_threadpool != nullptr);
    
    if (!used_pool) {
        thread_for_handling_process_output = std::thread([command_copy=this->command, stdout_ptr, running, handle_for_reading_output] {
            DEBUG_LOG("Started output reader thread for '{}'", command_copy);
            char buf[PROCESS_BUFFER_SIZE];
            DWORD n = 0;
            while (*running) {
                if (!ReadFile(handle_for_reading_output, buf, DWORD(sizeof(buf)), &n, nullptr) || n == 0) break;
                stdout_ptr->append(buf, buf + n);
            }
            DEBUG_LOG("Output reader thread finished for '{}'", command_copy);
        });
        thread_for_handling_process_error = std::thread([command_copy=this->command, stderr_ptr, running, handle_for_reading_error] {
            DEBUG_LOG("Started error reader thread for '{}'", command_copy);
            char buf[PROCESS_BUFFER_SIZE];
            DWORD n = 0;
            while (*running) {
                if (!ReadFile(handle_for_reading_error, buf, DWORD(sizeof(buf)), &n, nullptr) || n == 0) break;
                stderr_ptr->append(buf, buf + n);
            }
            DEBUG_LOG("Error reader thread finished for '{}'", command_copy);
        });
    } else {
        DEBUG_LOG("Enqueuing reader tasks into thread pool for '{}'", command);
        future_out = this->p_secondary_threadpool->enqueue([command_copy=this->command, stdout_ptr, running, handle_for_reading_output]() {
            DEBUG_LOG("Started output reader task for '{}'", command_copy);
            char buf[PROCESS_BUFFER_SIZE];
            DWORD n = 0;
            while (*running) {
                if (!ReadFile(handle_for_reading_output, buf, DWORD(sizeof(buf)), &n, nullptr) || n == 0) break;
                stdout_ptr->append(buf, buf + n);
            }
            DEBUG_LOG("Output reader task finished for '{}'", command_copy);
        });
        future_err = this->p_secondary_threadpool->enqueue([command_copy=this->command, stderr_ptr, running, handle_for_reading_error]() {
            DEBUG_LOG("Started error reader task for '{}'", command_copy);
            char buf[PROCESS_BUFFER_SIZE];
            DWORD n = 0;
            while (*running) {
                if (!ReadFile(handle_for_reading_error, buf, DWORD(sizeof(buf)), &n, nullptr) || n == 0) break;
                stderr_ptr->append(buf, buf + n);
            }
            DEBUG_LOG("Error reader task finished for '{}'", command_copy);
        });
    }

    DWORD waitTime = (timeout.count() > 0) ? DWORD(timeout.count()) : INFINITE;
    DEBUG_LOG("Waiting for process '{}' with timeout {} ms", command, timeout.count());
    DWORD result = WaitForSingleObject(pi.hProcess, waitTime);

    if (result == WAIT_TIMEOUT) {
        ERROR_LOG("Process '{}' timed out, terminating...", command);
        TerminateProcess(pi.hProcess, 1);
        WaitForSingleObject(pi.hProcess, INFINITE);
        DEBUG_LOG("Process '{}' terminated due to timeout", command);
        timed_out = true;
    } else {
        DEBUG_LOG("Process '{}' completed normally", command);
        timed_out = false;
    }

    *running = false;
    CloseHandle(handle_for_reading_output);
    CloseHandle(handle_for_reading_error);

    if (!used_pool) {
        if (thread_for_handling_process_output.joinable()) thread_for_handling_process_output.join();
        if (thread_for_handling_process_error.joinable()) thread_for_handling_process_error.join();
    } else {
        if (future_out.valid()) future_out.wait();
        if (future_err.valid()) future_err.wait();
    }

    DWORD exitCode = DWORD(-1);
    GetExitCodeProcess(pi.hProcess, &exitCode);
    DEBUG_LOG("Process '{}' exited with code {}", command, exitCode);

    // Removed atomic operation
    exit_code = static_cast<int>(exitCode);
    stdout_str = std::move(*stdout_ptr);
    stderr_str = std::move(*stderr_ptr);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return !timed_out;
}
#else
bool ProcessManager::executeInternal() {
    DEBUG_LOG("Preparing to run process on POSIX: '{}'", command);

    int pipe_out[2]{-1,-1}, pipe_error[2]{-1,-1};
    if (pipe(pipe_out) == -1 || pipe(pipe_error) == -1) {
        ERROR_LOG("Failed to create pipes for '{}'", command);
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        ERROR_LOG("Failed to fork process '{}'", command);
        close(pipe_out[0]); close(pipe_out[1]);
        close(pipe_error[0]); close(pipe_error[1]);
        return false;
    }

    if (pid == 0) {
        setsid();
        dup2(pipe_out[1], STDOUT_FILENO);
        dup2(pipe_error[1], STDERR_FILENO);

        close(pipe_out[0]); close(pipe_out[1]);
        close(pipe_error[0]); close(pipe_error[1]);

        execl("/bin/sh", "sh", "-c", command.c_str(), (char*)nullptr);
        _exit(127);
    }

    // Removed atomic operation
    process_id = pid;
    DEBUG_LOG("Forked process '{}' successfully (PID={})", command, pid);

    // Invoke creation callback
    if (this->creation_callback) {
        try {
            creation_callback(process_id);
        } catch (...) {
            ERROR_LOG("Creation callback threw an exception for process '{}'", command);
        }
    }

    close(pipe_out[1]);
    close(pipe_error[1]);

    auto stdout_ptr = std::make_shared<std::string>();
    auto stderr_ptr = std::make_shared<std::string>();
    // Simplified - just use a regular bool instead of atomic
    auto running = std::make_shared<bool>(true);

    bool used_pool = (p_secondary_threadpool != nullptr);
    std::thread thread_for_handling_process_output, thread_for_handling_process_error;
    std::future<void> future_out, future_err;
    
    if (!used_pool) {
        thread_for_handling_process_output = std::thread([command_copy=this->command, stdout_ptr, running, pipe_out] {
            DEBUG_LOG("Started output reader thread for '{}'", command_copy);
            char buf[PROCESS_BUFFER_SIZE];
            ssize_t n;
            while (*running) {
                n = ::read(pipe_out[0], buf, sizeof(buf));
                if (n <= 0) break;
                stdout_ptr->append(buf, size_t(n));
            }
            DEBUG_LOG("Output reader thread finished for '{}'", command_copy);
        });
        thread_for_handling_process_error = std::thread([command_copy=this->command, running, stderr_ptr, pipe_error] {
            DEBUG_LOG("Started error reader thread for '{}'", command_copy);
            char buf[PROCESS_BUFFER_SIZE];
            ssize_t n;
            while (*running) {
                n = ::read(pipe_error[0], buf, sizeof(buf));
                if (n <= 0) break;
                stderr_ptr->append(buf, size_t(n));
            }
            DEBUG_LOG("Error reader thread finished for '{}'", command_copy);
        });
    } else {
        DEBUG_LOG("Enqueuing reader tasks into thread pool for '{}'", command);
        future_out = this->p_secondary_threadpool->enqueue([command_copy=this->command, running, stdout_ptr, pipe_out]() {
            DEBUG_LOG("Started output reader task for '{}'", command_copy);
            char buf[PROCESS_BUFFER_SIZE];
            ssize_t n;
            while (*running) {
                n = ::read(pipe_out[0], buf, sizeof(buf));
                if (n <= 0) break;
                stdout_ptr->append(buf, size_t(n));
            }
            DEBUG_LOG("Output reader task finished for '{}'", command_copy);
        });
        future_err = this->p_secondary_threadpool->enqueue([command_copy=this->command, running, stderr_ptr, pipe_error]() {
            DEBUG_LOG("Started error reader thread for '{}'", command_copy);
            char buf[PROCESS_BUFFER_SIZE];
            ssize_t n;
            while (*running) {
                n = ::read(pipe_error[0], buf, sizeof(buf));
                if (n <= 0) break;
                stderr_ptr->append(buf, size_t(n));
            }
            DEBUG_LOG("Error reader thread finished for '{}'", command_copy);
        });
    }

    int status = 0;
    timed_out = false;

    if (timeout.count() > 0) {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        DEBUG_LOG("Waiting for process '{}' with timeout {} ms", command, timeout.count());
        while (true) {
            pid_t ret = waitpid(pid, &status, WNOHANG);
            if (ret == pid) break;
            if (ret == -1) {
                ERROR_LOG("waitpid failed for '{}'", command);
                break;
            }
            if (std::chrono::steady_clock::now() >= deadline) {
                ERROR_LOG("Process '{}' timed out, killing...", command);
                kill(-pid, SIGKILL);
                timed_out = true;
                (void)waitpid(pid, &status, 0);
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    } else {
        DEBUG_LOG("Waiting indefinitely for process '{}'", command);
        (void)waitpid(pid, &status, 0);
    }

    *running = false;
    close(pipe_out[0]);
    close(pipe_error[0]);

    if (!used_pool) {
        if (thread_for_handling_process_output.joinable()) thread_for_handling_process_output.join();
        if (thread_for_handling_process_error.joinable()) thread_for_handling_process_error.join();
    } else {
        if (future_out.valid()) future_out.wait();
        if (future_err.valid()) future_err.wait();
    }

    int exit_code_value = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    DEBUG_LOG("Process '{}' exited with code {}", command, exit_code_value);

    // Removed atomic operation
    exit_code = exit_code_value;
    stdout_str = std::move(*stdout_ptr);
    stderr_str = std::move(*stderr_ptr);

    return !timed_out;
}
#endif

void ProcessManager::execute() {
    // Validate state before execution
    if (command.empty()) {
        ERROR_LOG("Cannot execute: empty command");
        return;
    }
    this->executeInternal();
    DEBUG_LOG("EXIT_CODE: {}\nSTDOUT: {}\nSTDERR: {}", this->exit_code, this->stdout_str, this->stderr_str);
}

} // namespace rat::process
