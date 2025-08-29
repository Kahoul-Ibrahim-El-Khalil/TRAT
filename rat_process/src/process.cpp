#include "rat/process.hpp"
#include <thread>
#include <chrono>
#include <optional>
#include <vector>
#include <mutex>
#include <memory>
#include <string>
#include <functional>

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

// --- Implementation ----------------------------------------------------------
void runAsyncProcess(ProcessContext& Process_Context) {
    const std::string command = Process_Context.command; // safe copy for async
    const auto timeout = Process_Context.timeout;
    auto mutexes = Process_Context.mutexes;              // optional<vector<mutex*> or ref_wrapper>
    auto callback = Process_Context.callback;            // copy callable

    Process_Context.thread_pool->enqueue([command, timeout, mutexes, callback]() {
        std::vector<std::unique_lock<std::mutex>> locks;

        if (mutexes && !mutexes->empty()) {
            std::vector<std::mutex*> ptrs;
            ptrs.reserve(mutexes->size());
            for (auto mptr : *mutexes) {
                if (mptr) ptrs.push_back(mptr);
            }

            if (!ptrs.empty()) {
                std::sort(ptrs.begin(), ptrs.end());

                locks.reserve(ptrs.size());
                // Lock each mutex in the canonical order
                for (auto mptr : ptrs) {
                    // construct a unique_lock that immediately locks the mutex
                    locks.emplace_back(*mptr);
                }
                // now all mutexes in `ptrs` are locked and will be unlocked when `locks` goes out of scope
            }
        }
        auto safe_invoke = [&](std::optional<ProcessResult> result) {
            try { callback(std::move(result)); }
            catch (...) { /* swallow: never throw out of worker */ }
        };

#ifdef _WIN32
        // --- Windows path ----------------------------------------------------
        SECURITY_ATTRIBUTES sa{ sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
        HANDLE hOutRead = nullptr, hOutWrite = nullptr;
        HANDLE hErrRead = nullptr, hErrWrite = nullptr;

        if (!CreatePipe(&hOutRead, &hOutWrite, &sa, 0) ||
            !CreatePipe(&hErrRead, &hErrWrite, &sa, 0)) {
            safe_invoke(std::nullopt);
            return;
        }

        // Ensure the parent-side read ends are NOT inheritable
        SetHandleInformation(hOutRead, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(hErrRead, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA si{};
        si.cb = sizeof(si);
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdOutput = hOutWrite;
        si.hStdError  = hErrWrite;

        PROCESS_INFORMATION pi{};
        // Inherit handles so child gets write ends
        BOOL ok = CreateProcessA(
            nullptr,
            const_cast<LPSTR>(command.c_str()),
            nullptr, nullptr, TRUE, /* inherit handles */
            0, nullptr,
            &si, &pi
        );

        if (!ok) {
            CloseHandle(hOutRead); CloseHandle(hOutWrite);
            CloseHandle(hErrRead); CloseHandle(hErrWrite);
            safe_invoke(std::nullopt);
            return;
        }

        // Parent no longer needs the write ends
        CloseHandle(hOutWrite);
        CloseHandle(hErrWrite);

        std::string stdoutStr, stderrStr;
        std::atomic<bool> running{true};

        auto reader = [&](HANDLE hRead, std::string& out) {
            char buf[4096];
            DWORD n = 0;
            while (running.load()) {
                if (!ReadFile(hRead, buf, DWORD(sizeof(buf)), &n, nullptr) || n == 0) break;
                out.append(buf, buf + n);
            }
        };

        std::thread tOut(reader, hOutRead, std::ref(stdoutStr));
        std::thread tErr(reader, hErrRead, std::ref(stderrStr));

        DWORD waitTime = (timeout.count() > 0) ? DWORD(timeout.count()) : INFINITE;
        DWORD result = WaitForSingleObject(pi.hProcess, waitTime);

        if (result == WAIT_TIMEOUT) {
            // Kill the process; readers will exit when pipe is closed
            TerminateProcess(pi.hProcess, 1);
        }

        // Wait for process to actually end (avoid zombie)
        WaitForSingleObject(pi.hProcess, INFINITE);
        running.store(false);

        // Closing read handles will break the readers if they’re still blocked
        CloseHandle(hOutRead);
        CloseHandle(hErrRead);

        tOut.join();
        tErr.join();

        DWORD exitCode = DWORD(-1);
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (result == WAIT_TIMEOUT) {
            safe_invoke(std::nullopt); // treat timeout as no-result
            return;
        }

        safe_invoke(ProcessResult{ int(exitCode), std::move(stdoutStr), std::move(stderrStr) });

#else
        // --- POSIX path ------------------------------------------------------
        int pipeOut[2]{-1,-1}, pipeErr[2]{-1,-1};
        if (pipe(pipeOut) == -1 || pipe(pipeErr) == -1) {
            if (pipeOut[0] != -1) { close(pipeOut[0]); close(pipeOut[1]); }
            if (pipeErr[0] != -1) { close(pipeErr[0]); close(pipeErr[1]); }
            safe_invoke(std::nullopt);
            return;
        }

        pid_t pid = fork();
        if (pid < 0) {
            close(pipeOut[0]); close(pipeOut[1]);
            close(pipeErr[0]); close(pipeErr[1]);
            safe_invoke(std::nullopt);
            return;
        }

        if (pid == 0) {
            // Child
            // Create a new process group so we can kill the whole tree
            setsid();

            dup2(pipeOut[1], STDOUT_FILENO);
            dup2(pipeErr[1], STDERR_FILENO);

            close(pipeOut[0]); close(pipeOut[1]);
            close(pipeErr[0]); close(pipeErr[1]);

            // Use /bin/sh -c to run the command string
            execl("/bin/sh", "sh", "-c", command.c_str(), (char*)nullptr);
            _exit(127); // exec failed
        }

        // Parent
        close(pipeOut[1]);
        close(pipeErr[1]);

        std::string stdoutStr, stderrStr;
        std::atomic<bool> running{true};

        auto reader = [&](int fd, std::string& out) {
            char buf[4096];
            ssize_t n;
            while (running.load()) {
                n = ::read(fd, buf, sizeof(buf));
                if (n <= 0) break;
                out.append(buf, size_t(n));
            }
        };

        std::thread tOut(reader, pipeOut[0], std::ref(stdoutStr));
        std::thread tErr(reader, pipeErr[0], std::ref(stderrStr));

        int status = 0;
        bool timedOut = false;

        if (timeout.count() > 0) {
            auto deadline = std::chrono::steady_clock::now() + timeout;
            while (true) {
                pid_t ret = waitpid(pid, &status, WNOHANG);
                if (ret == pid) break;
                if (ret == -1) { /* error */ break; }
                if (std::chrono::steady_clock::now() >= deadline) {
                    // Kill the *process group*
                    kill(-pid, SIGKILL);
                    timedOut = true;
                    // Ensure it’s reaped
                    (void)waitpid(pid, &status, 0);
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        } else {
            (void)waitpid(pid, &status, 0);
        }

        running.store(false);
        close(pipeOut[0]);
        close(pipeErr[0]);
        tOut.join();
        tErr.join();

        if (timedOut) {
            safe_invoke(std::nullopt);
            return;
        }

        int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        safe_invoke(ProcessResult{ exitCode, std::move(stdoutStr), std::move(stderrStr) });
#endif
    });
}

} // namespace rat::process

