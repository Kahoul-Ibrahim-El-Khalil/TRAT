#include "rat/process.hpp"
#include <thread>
#include <chrono>
#include <optional>
#include <vector>
#include <mutex>
#include <memory>
#include <string>
#include <functional>
#include <atomic>
#include <future>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#endif

#include "rat/ThreadPool.hpp"

#ifdef DEBUG_RAT_PROCESS
    #include <spdlog/spdlog.h>
    #include <spdlog/sinks/stdout_color_sinks.h>

    namespace {
        struct _rat_logging_initializer {
            _rat_logging_initializer() {
                spdlog::set_level(spdlog::level::debug);
                spdlog::set_pattern("[%H:%M:%S] [%^%l%$] %v");
            }
        } _rat_logging_initializer_instance;
    }

    #define DEBUG_LOG(...) spdlog::debug(__VA_ARGS__)
    #define ERROR_LOG(...) spdlog::error(__VA_ARGS__)
#else
    #define DEBUG_LOG(...) ((void)0)
    #define ERROR_LOG(...) ((void)0)
#endif

namespace rat::process {

constexpr size_t PROCESS_BUFFER_SIZE = 512;

#ifdef _WIN32
static std::optional<ProcessResult> runProcessWindows(
    const std::string& arg_Command,
    std::atomic<pid_t>& Process_Id,
    std::chrono::milliseconds arg_Timeout,
    ::rat::threading::ThreadPool* p_Thread_Pool_For_Handling_Output_And_Error = nullptr
    )
{
    DEBUG_LOG("Preparing to run process on Windows: '{}'", arg_Command);

    SECURITY_ATTRIBUTES security_attribute{ sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
    HANDLE handle_for_reading_output = nullptr, 
    HANDLE handle_for_writing_output = nullptr;
    
    HANDLE handle_for_reading_error  = nullptr; 
    HANDLE handle_for_writing_error  = nullptr;

    if (!CreatePipe(&handle_for_reading_output, &handle_for_writing_output, &security_attribute, 0) ||
        !CreatePipe(&handle_for_reading_error,  &handle_for_writing_error,  &security_attribute, 0)) {
        ERROR_LOG("Failed to create pipes for process '{}'", arg_Command);
        return std::nullopt;
    }

    SetHandleInformation(handle_for_reading_output, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(handle_for_reading_error,  HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = handle_for_writing_output;
    si.hStdError  = handle_for_writing_error;

    PROCESS_INFORMATION pi{};
    BOOL ok = CreateProcessA(
        nullptr,
        const_cast<LPSTR>(arg_Command.c_str()),
        nullptr, nullptr, TRUE,       // inherit handles
        0, nullptr, nullptr,          // creation flags, env, current dir
        &si, &pi
    );

    if (!ok) {
        ERROR_LOG("Failed to create process: '{}', error code {}", arg_Command, GetLastError());
        CloseHandle(handle_for_reading_output); CloseHandle(handle_for_writing_output);
        CloseHandle(handle_for_reading_error);  CloseHandle(handle_for_writing_error);
        return std::nullopt;
    }
    Process_Id.store(static_cast<pid_t>(pi.dwProcessId), std::memory_order_relaxed);
    DEBUG_LOG("Process '{}' created successfully (PID={})", arg_Command, pi.dwProcessId);

    CloseHandle(handle_for_writing_output);
    CloseHandle(handle_for_writing_error);

    auto stdout_ptr = std::make_shared<std::string>();
    auto stderr_ptr = std::make_shared<std::string>();
    auto running = std::make_shared<std::atomic<bool>>(true);

    std::thread thread_for_handling_process_output, thread_for_handling_process_error;
    std::future<void> future_out, future_err;
    bool used_pool = (p_Thread_Pool_For_Handling_Output_And_Error != nullptr);

    if (!used_pool) {
        thread_for_handling_process_output = std::thread([&, running, stdout_ptr] { 
            DEBUG_LOG("Started output reader thread for '{}'", arg_Command);
            char buf[PROCESS_BUFFER_SIZE];
            DWORD n = 0;
            while (running->load()) {
                if (!ReadFile(handle_for_reading_output, buf, DWORD(sizeof(buf)), &n, nullptr) || n == 0) break;
                stdout_ptr->append(buf, buf + n);
            }
            DEBUG_LOG("Output reader thread finished for '{}'", arg_Command);
        });
        thread_for_handling_process_error = std::thread([&, running, stderr_ptr] { 
            DEBUG_LOG("Started error reader thread for '{}'", arg_Command);
            char buf[PROCESS_BUFFER_SIZE];
            DWORD n = 0;
            while (running->load()) {
                if (!ReadFile(handle_for_reading_error, buf, DWORD(sizeof(buf)), &n, nullptr) || n == 0) break;
                stderr_ptr->append(buf, buf + n);
            }
            DEBUG_LOG("Error reader thread finished for '{}'", arg_Command);
        });
    } else {
        DEBUG_LOG("Enqueuing reader tasks into thread pool for '{}'", arg_Command);
        future_out = p_Thread_Pool_For_Handling_Output_And_Error->enqueue([h = handle_for_reading_output, stdout_ptr, running, arg_Command]() {
            DEBUG_LOG("Started output reader task for '{}'", arg_Command);
            char buf[PROCESS_BUFFER_SIZE];
            DWORD n = 0;
            while (running->load()) {
                if (!ReadFile(h, buf, DWORD(sizeof(buf)), &n, nullptr) || n == 0) break;
                stdout_ptr->append(buf, buf + n);
            }
            DEBUG_LOG("Output reader task finished for '{}'", arg_Command);
        });
        future_err = p_Thread_Pool_For_Handling_Output_And_Error->enqueue([h = handle_for_reading_error, stderr_ptr, running, arg_Command]() {
            DEBUG_LOG("Started error reader task for '{}'", arg_Command);
            char buf[PROCESS_BUFFER_SIZE];
            DWORD n = 0;
            while (running->load()) {
                if (!ReadFile(h, buf, DWORD(sizeof(buf)), &n, nullptr) || n == 0) break;
                stderr_ptr->append(buf, buf + n);
            }
            DEBUG_LOG("Error reader task finished for '{}'", arg_Command);
        });
    }

    DWORD waitTime = (arg_Timeout.count() > 0) ? DWORD(arg_Timeout.count()) : INFINITE;
    DEBUG_LOG("Waiting for process '{}' with timeout {} ms", arg_Command, arg_Timeout.count());
    DWORD result = WaitForSingleObject(pi.hProcess, waitTime);

    if (result == WAIT_TIMEOUT) {
        ERROR_LOG("Process '{}' timed out, terminating...", arg_Command);
        TerminateProcess(pi.hProcess, 1);
        WaitForSingleObject(pi.hProcess, INFINITE);
        DEBUG_LOG("Process '{}' terminated due to timeout", arg_Command);
    
        running->store(false);
        CloseHandle(handle_for_reading_output);
        CloseHandle(handle_for_reading_error);

        if (!used_pool) {
            if (thread_for_handling_process_output.joinable()) thread_for_handling_process_output.join();
            if (thread_for_handling_process_error.joinable()) thread_for_handling_process_error.join();
        } else {
            if (future_out.valid()) future_out.wait();
            if (future_err.valid()) future_err.wait();
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return std::nullopt;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DEBUG_LOG("Process '{}' completed normally", arg_Command);

    running->store(false);
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
    DEBUG_LOG("Process '{}' exited with code {}", arg_Command, exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return ProcessResult{ int(exitCode), std::move(*stdout_ptr), std::move(*stderr_ptr) };
}
#endif // _WIN32

#ifndef _WIN32
static std::optional<ProcessResult> runProcessPOSIX(
    const std::string& arg_Command,
    std::atomic<pid_t>& Process_Id,
    std::chrono::milliseconds arg_Timeout,
    ::rat::threading::ThreadPool* p_Thread_Pool_For_Handling_Output_And_Error = nullptr)
{
    DEBUG_LOG("Preparing to run process on POSIX: '{}'", arg_Command);

    int pipe_out[2]{-1,-1}, pipe_error[2]{-1,-1};
    if (pipe(pipe_out) == -1 || pipe(pipe_error) == -1) {
        ERROR_LOG("Failed to create pipes for '{}'", arg_Command);
        if (pipe_out[0] != -1) { close(pipe_out[0]); close(pipe_out[1]); }
        if (pipe_error[0] != -1) { close(pipe_error[0]); close(pipe_error[1]); }
        return std::nullopt;
    }

    pid_t pid = fork();
    if (pid < 0) {
        ERROR_LOG("Failed to fork process '{}'", arg_Command);
        close(pipe_out[0]); close(pipe_out[1]);
        close(pipe_error[0]); close(pipe_error[1]);
        return std::nullopt;
    }

    if (pid == 0) {
        setsid();
        dup2(pipe_out[1], STDOUT_FILENO);
        dup2(pipe_error[1], STDERR_FILENO);

        close(pipe_out[0]); close(pipe_out[1]);
        close(pipe_error[0]); close(pipe_error[1]);

        execl("/bin/sh", "sh", "-c", arg_Command.c_str(), (char*)nullptr);
        _exit(127);
    }
    Process_Id.store(pid, std::memory_order_relaxed);
    DEBUG_LOG("Forked process '{}' successfully (PID={})", arg_Command, pid);

    close(pipe_out[1]);
    close(pipe_error[1]);

    auto stdout_ptr = std::make_shared<std::string>();
    auto stderr_ptr = std::make_shared<std::string>();
    auto running = std::make_shared<std::atomic<bool>>(true);

    bool used_pool = (p_Thread_Pool_For_Handling_Output_And_Error != nullptr);
    std::thread thread_for_handling_process_output, thread_for_handling_process_error;
    std::future<void> future_out, future_err;

    if (!used_pool) {
        thread_for_handling_process_output = std::thread([&, running, stdout_ptr] { 
            DEBUG_LOG("Started output reader thread for '{}'", arg_Command);
            char buf[PROCESS_BUFFER_SIZE];
            ssize_t n;
            while (running->load()) {
                n = ::read(pipe_out[0], buf, sizeof(buf));
                if (n <= 0) break;
                stdout_ptr->append(buf, size_t(n));
            }
            DEBUG_LOG("Output reader thread finished for '{}'", arg_Command);
        });
        thread_for_handling_process_error = std::thread([&, running, stderr_ptr] { 
            DEBUG_LOG("Started error reader thread for '{}'", arg_Command);
            char buf[PROCESS_BUFFER_SIZE];
            ssize_t n;
            while (running->load()) {
                n = ::read(pipe_error[0], buf, sizeof(buf));
                if (n <= 0) break;
                stderr_ptr->append(buf, size_t(n));
            }
            DEBUG_LOG("Error reader thread finished for '{}'", arg_Command);
        });
    } else {
        DEBUG_LOG("Enqueuing reader tasks into thread pool for '{}'", arg_Command);
        future_out = p_Thread_Pool_For_Handling_Output_And_Error->enqueue([fd = pipe_out[0], stdout_ptr, running, arg_Command]() {
            DEBUG_LOG("Started output reader task for '{}'", arg_Command);
            char buf[PROCESS_BUFFER_SIZE];
            ssize_t n;
            while (running->load()) {
                n = ::read(fd, buf, sizeof(buf));
                if (n <= 0) break;
                stdout_ptr->append(buf, size_t(n));
            }
            DEBUG_LOG("Output reader task finished for '{}'", arg_Command);
        });
        future_err = p_Thread_Pool_For_Handling_Output_And_Error->enqueue([fd = pipe_error[0], stderr_ptr, running, arg_Command]() {
            DEBUG_LOG("Started error reader task for '{}'", arg_Command);
            char buf[PROCESS_BUFFER_SIZE];
            ssize_t n;
            while (running->load()) {
                n = ::read(fd, buf, sizeof(buf));
                if (n <= 0) break;
                stderr_ptr->append(buf, size_t(n));
            }
            DEBUG_LOG("Error reader task finished for '{}'", arg_Command);
        });
    }

    int status = 0;
    bool timed_out = false;

    if (arg_Timeout.count() > 0) {
        auto deadline = std::chrono::steady_clock::now() + arg_Timeout;
        DEBUG_LOG("Waiting for process '{}' with timeout {} ms", arg_Command, arg_Timeout.count());
        while (true) {
            pid_t ret = waitpid(pid, &status, WNOHANG);
            if (ret == pid) break;
            if (ret == -1) {
                ERROR_LOG("waitpid failed for '{}'", arg_Command);
                break;
            }
            if (std::chrono::steady_clock::now() >= deadline) {
                ERROR_LOG("Process '{}' timed out, killing...", arg_Command);
                kill(-pid, SIGKILL);
                timed_out = true;
                (void)waitpid(pid, &status, 0);
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    } else {
        DEBUG_LOG("Waiting indefinitely for process '{}'", arg_Command);
        (void)waitpid(pid, &status, 0);
    }

    running->store(false);
    close(pipe_out[0]);
    close(pipe_error[0]);

    if (!used_pool) {
        if (thread_for_handling_process_output.joinable()) thread_for_handling_process_output.join();
        if (thread_for_handling_process_error.joinable()) thread_for_handling_process_error.join();
    } else {
        if (future_out.valid()) future_out.wait();
        if (future_err.valid()) future_err.wait();
    }

    if (timed_out) return std::nullopt;

    int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    DEBUG_LOG("Process '{}' exited with code {}", arg_Command, exit_code);
    return ProcessResult{ exit_code, std::move(*stdout_ptr), std::move(*stderr_ptr) };
}
#endif // !_WIN32

void runAsyncProcess(ProcessContext& Process_Context, std::atomic<pid_t>& Process_Id) {
    DEBUG_LOG("Scheduling async process: '{}'", Process_Context.command);
    
    // Create a shared pointer to capture the context safely
    auto sPtr_context = std::make_shared<ProcessContext>(std::move(Process_Context));
    
    Process_Context.thread_pool->enqueue(
        [sPtr_context, &Process_Id]() {  // Capture by shared_ptr and reference
            DEBUG_LOG("Async process task started for '{}'", sPtr_context->command);

            std::vector<std::unique_lock<std::mutex>> locks;

            if (sPtr_context->mutexes && !sPtr_context->mutexes->empty()) {
                DEBUG_LOG("Locking {} mutexes for '{}'", sPtr_context->mutexes->size(), sPtr_context->command);
                std::vector<std::mutex*> ptrs;
                ptrs.reserve(sPtr_context->mutexes->size());
                for (auto mptr : *sPtr_context->mutexes)
                    if (mptr) ptrs.push_back(mptr);

                if (!ptrs.empty()) {
                    std::sort(ptrs.begin(), ptrs.end());
                    locks.reserve(ptrs.size());
                    for (auto mptr : ptrs)
                        locks.emplace_back(*mptr);
                }
            }

            auto safe_invoke = [&](std::optional<ProcessResult> result) {
                try {
                    DEBUG_LOG("Invoking callback for '{}'", sPtr_context->command);
                    sPtr_context->callback(std::move(result));
                }
                catch (...) {
                    ERROR_LOG("Callback for '{}' threw an exception", sPtr_context->command);
                }
            };

#ifdef _WIN32
            safe_invoke(runProcessWindows(sPtr_context->command, 
                                          Process_Id,
                                          sPtr_context->timeout, 
                                          sPtr_context->secondary_thread_pool
                                          ));
#else
            safe_invoke(runProcessPOSIX(sPtr_context->command, 
                                        Process_Id,
                                        sPtr_context->timeout, 
                                        sPtr_context->secondary_thread_pool
                                        ));
#endif

            DEBUG_LOG("Async process task finished for '{}'", sPtr_context->command);
        }
    );
}
} // namespace rat::process

#undef DEBUG_LOG
#undef ERROR_LOG
#ifdef DEBUG_RAT_PROCESS
    #undef DEBUG_RAT_PROCESS
#endif
