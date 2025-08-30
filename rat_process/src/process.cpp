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

namespace rat::process {

constexpr size_t PROCESS_BUFFER_SIZE = 512;

// --- Windows helper ---------------------------------------------------------
#ifdef _WIN32
static std::optional<ProcessResult> runProcessWindows(
    const std::string& arg_Command,
    std::chrono::milliseconds arg_Timeout,
    ::rat::threading::ThreadPool* p_Thread_Pool_For_Handling_Output_And_Error = nullptr)
{
    SECURITY_ATTRIBUTES security_attribute{ sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
    HANDLE handle_for_reading_output = nullptr, handle_for_writing_output = nullptr;
    HANDLE handle_for_reading_error  = nullptr, handle_for_writing_error  = nullptr;

    if (!CreatePipe(&handle_for_reading_output, &handle_for_writing_output, &security_attribute, 0) ||
        !CreatePipe(&handle_for_reading_error,  &handle_for_writing_error,  &security_attribute, 0)) {
        return std::nullopt;
    }

    // Parent should not inherit the read end.
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
        CloseHandle(handle_for_reading_output); CloseHandle(handle_for_writing_output);
        CloseHandle(handle_for_reading_error);  CloseHandle(handle_for_writing_error);
        return std::nullopt;
    }

    CloseHandle(handle_for_writing_output);
    CloseHandle(handle_for_writing_error);

    // Use shared_ptr to safely share lifetime with pool tasks (if any)
    auto stdout_ptr = std::make_shared<std::string>();
    auto stderr_ptr = std::make_shared<std::string>();
    auto running = std::make_shared<std::atomic<bool>>(true);

    DWORD result = 0;
    // Either spawn two threads (fallback) or enqueue two tasks into pool
    std::thread thread_for_handling_process_output, thread_for_handling_process_error;
    std::future<void> future_out, future_err;
    bool used_pool = (p_Thread_Pool_For_Handling_Output_And_Error != nullptr);
    
    if (!used_pool) {
        // spawn threads (previous behavior)
        thread_for_handling_process_output = std::thread([&, running, stdout_ptr] { 
            char buf[PROCESS_BUFFER_SIZE];
            DWORD n = 0;
            while (running->load()) {
                if (!ReadFile(handle_for_reading_output, buf, DWORD(sizeof(buf)), &n, nullptr) || n == 0) break;
                stdout_ptr->append(buf, buf + n);
            }
        });
        thread_for_handling_process_error = std::thread([&, running, stderr_ptr] { 
            char buf[PROCESS_BUFFER_SIZE];
            DWORD n = 0;
            while (running->load()) {
                if (!ReadFile(handle_for_reading_error, buf, DWORD(sizeof(buf)), &n, nullptr) || n == 0) break;
                stderr_ptr->append(buf, buf + n);
            }
        });
    } else {
        // enqueue into provided pool — capture copies to keep lifetime safe
        future_out = p_Thread_Pool_For_Handling_Output_And_Error->enqueue([h = handle_for_reading_output, stdout_ptr, running]() {
            char buf[PROCESS_BUFFER_SIZE];
            DWORD n = 0;
            while (running->load()) {
                if (!ReadFile(h, buf, DWORD(sizeof(buf)), &n, nullptr) || n == 0) break;
                stdout_ptr->append(buf, buf + n);
            }
        });
        future_err = p_Thread_Pool_For_Handling_Output_And_Error->enqueue([h = handle_for_reading_error, stderr_ptr, running]() {
            char buf[PROCESS_BUFFER_SIZE];
            DWORD n = 0;
            while (running->load()) {
                if (!ReadFile(h, buf, DWORD(sizeof(buf)), &n, nullptr) || n == 0) break;
                stderr_ptr->append(buf, buf + n);
            }
        });
    }

    DWORD waitTime = (arg_Timeout.count() > 0) ? DWORD(arg_Timeout.count()) : INFINITE;
    result = WaitForSingleObject(pi.hProcess, waitTime);

    // Handle timeout case with proper cleanup
    if (result == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        // Wait for the process to actually terminate
        WaitForSingleObject(pi.hProcess, INFINITE);
    
        // Signal readers and cleanup
        running->store(false);
        CloseHandle(handle_for_reading_output);
        CloseHandle(handle_for_reading_error);
    
        if (!used_pool) {
            if (thread_for_handling_process_output.joinable()) thread_for_handling_process_output.join();
            if (thread_for_handling_process_error.joinable()) thread_for_handling_process_error.join();
        } else {
            // Wait for pool tasks to complete
            if (future_out.valid()) future_out.wait();
            if (future_err.valid()) future_err.wait();
        }
    
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    
        return std::nullopt;
    }

    // Ensure process has ended for non-timeout case
    WaitForSingleObject(pi.hProcess, INFINITE);

    // signal readers to stop and close read handles (this will usually break ReadFile)
    running->store(false);
    CloseHandle(handle_for_reading_output);
    CloseHandle(handle_for_reading_error);

    if (!used_pool) {
        // join fallback threads
        if (thread_for_handling_process_output.joinable()) thread_for_handling_process_output.join();
        if (thread_for_handling_process_error.joinable()) thread_for_handling_process_error.join();
    } else {
        // Wait for pool tasks to complete
        if (future_out.valid()) future_out.wait();
        if (future_err.valid()) future_err.wait();
    }

    DWORD exitCode = DWORD(-1);
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return ProcessResult{ int(exitCode), std::move(*stdout_ptr), std::move(*stderr_ptr) };
}
#endif // _WIN32

// --- POSIX helper -----------------------------------------------------------
#ifndef _WIN32
static std::optional<ProcessResult> runProcessPOSIX(
    const std::string& arg_Command,
    std::chrono::milliseconds arg_Timeout,
    ::rat::threading::ThreadPool* p_Thread_Pool_For_Handling_Output_And_Error = nullptr)
{
    int pipe_out[2]{-1,-1}, pipe_error[2]{-1,-1};
    if (pipe(pipe_out) == -1 || pipe(pipe_error) == -1) {
        if (pipe_out[0] != -1) { close(pipe_out[0]); close(pipe_out[1]); }
        if (pipe_error[0] != -1) { close(pipe_error[0]); close(pipe_error[1]); }
        return std::nullopt;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(pipe_out[0]); close(pipe_out[1]);
        close(pipe_error[0]); close(pipe_error[1]);
        return std::nullopt;
    }

    if (pid == 0) {
        setsid(); // new process group

        dup2(pipe_out[1], STDOUT_FILENO);
        dup2(pipe_error[1], STDERR_FILENO);

        close(pipe_out[0]); close(pipe_out[1]);
        close(pipe_error[0]); close(pipe_error[1]);

        execl("/bin/sh", "sh", "-c", arg_Command.c_str(), (char*)nullptr);
        _exit(127);
    }

    // Parent:
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
            char buf[PROCESS_BUFFER_SIZE];
            ssize_t n;
            while (running->load()) {
                n = ::read(pipe_out[0], buf, sizeof(buf));
                if (n <= 0) break;
                stdout_ptr->append(buf, size_t(n));
            }
        });
        thread_for_handling_process_error = std::thread([&, running, stderr_ptr] { 
            char buf[PROCESS_BUFFER_SIZE];
            ssize_t n;
            while (running->load()) {
                n = ::read(pipe_error[0], buf, sizeof(buf));
                if (n <= 0) break;
                stderr_ptr->append(buf, size_t(n));
            }
        });
    } else {
        // enqueue tasks to shared pool; capture copies to preserve lifetime
        future_out = p_Thread_Pool_For_Handling_Output_And_Error->enqueue([fd = pipe_out[0], stdout_ptr, running]() {
            char buf[PROCESS_BUFFER_SIZE];
            ssize_t n;
            while (running->load()) {
                n = ::read(fd, buf, sizeof(buf));
                if (n <= 0) break;
                stdout_ptr->append(buf, size_t(n));
            }
        });
        future_err = p_Thread_Pool_For_Handling_Output_And_Error->enqueue([fd = pipe_error[0], stderr_ptr, running]() {
            char buf[PROCESS_BUFFER_SIZE];
            ssize_t n;
            while (running->load()) {
                n = ::read(fd, buf, sizeof(buf));
                if (n <= 0) break;
                stderr_ptr->append(buf, size_t(n));
            }
        });
    }

    int status = 0;
    bool timed_out = false;

    if (arg_Timeout.count() > 0) {
        auto deadline = std::chrono::steady_clock::now() + arg_Timeout;
        while (true) {
            pid_t ret = waitpid(pid, &status, WNOHANG);
            if (ret == pid) break;
            if (ret == -1) break;
            if (std::chrono::steady_clock::now() >= deadline) {
                // kill group
                kill(-pid, SIGKILL);
                timed_out = true;
                (void)waitpid(pid, &status, 0);
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    } else {
        (void)waitpid(pid, &status, 0);
    }

    // signal readers to stop and close fds
    running->store(false);
    close(pipe_out[0]);
    close(pipe_error[0]);

    if (!used_pool) {
        if (thread_for_handling_process_output.joinable()) thread_for_handling_process_output.join();
        if (thread_for_handling_process_error.joinable()) thread_for_handling_process_error.join();
    } else {
        // Wait for pool tasks to complete
        if (future_out.valid()) future_out.wait();
        if (future_err.valid()) future_err.wait();
    }

    if (timed_out) return std::nullopt;

    int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return ProcessResult{ exit_code, std::move(*stdout_ptr), std::move(*stderr_ptr) };
}
#endif // !_WIN32

// --- Main async runner ------------------------------------------------------
void runAsyncProcess(ProcessContext& Process_Context) {
    Process_Context.thread_pool->enqueue(
        [Process_Context]()  // capture the context by value safely
        {
            std::vector<std::unique_lock<std::mutex>> locks;

            if (Process_Context.mutexes && !Process_Context.mutexes->empty()) {
                std::vector<std::mutex*> ptrs;
                ptrs.reserve(Process_Context.mutexes->size());
                for (auto mptr : *Process_Context.mutexes)
                    if (mptr) ptrs.push_back(mptr);

                if (!ptrs.empty()) {
                    std::sort(ptrs.begin(), ptrs.end());
                    locks.reserve(ptrs.size());
                    for (auto mptr : ptrs)
                        locks.emplace_back(*mptr);
                }
            }

            auto safe_invoke = [&](std::optional<ProcessResult> result) {
                try { Process_Context.callback(std::move(result)); }
                catch (...) { }
            };

#ifdef _WIN32
            safe_invoke(runProcessWindows(Process_Context.command, Process_Context.timeout, Process_Context.secondary_thread_pool));
#else
            safe_invoke(runProcessPOSIX(Process_Context.command, Process_Context.timeout, Process_Context.secondary_thread_pool));
#endif
        }
    );
}

} // namespace rat::process