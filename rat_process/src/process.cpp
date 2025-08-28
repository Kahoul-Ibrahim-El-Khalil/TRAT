#include "rat/process.hpp"
#include <thread>
#include <chrono>
#include <optional>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#endif

namespace rat::process {

void runAsyncProcess(const std::string& command,
                     std::chrono::milliseconds timeout,
                     ProcessCallback callback)
{
    std::thread([command, timeout, callback]() {
#ifdef _WIN32
        // Windows version
        SECURITY_ATTRIBUTES sa{ sizeof(sa), nullptr, TRUE };
        HANDLE hOutRead, hOutWrite, hErrRead, hErrWrite;
        if (!CreatePipe(&hOutRead, &hOutWrite, &sa, 0) ||
            !CreatePipe(&hErrRead, &hErrWrite, &sa, 0)) { callback(std::nullopt); return; }

        STARTUPINFO si{};
        si.cb = sizeof(si);
        si.hStdOutput = hOutWrite;
        si.hStdError = hErrWrite;
        si.dwFlags |= STARTF_USESTDHANDLES;

        PROCESS_INFORMATION pi{};
        if (!CreateProcess(nullptr, (LPSTR)command.c_str(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
            callback(std::nullopt);
            return;
        }

        CloseHandle(hOutWrite);
        CloseHandle(hErrWrite);

        DWORD waitTime = (timeout.count() > 0) ? (DWORD)timeout.count() : INFINITE;
        DWORD result = WaitForSingleObject(pi.hProcess, waitTime);
        if (result == WAIT_TIMEOUT) {
            TerminateProcess(pi.hProcess, 1);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            callback(std::nullopt);
            return;
        }

        std::string stdoutStr, stderrStr;
        char buf[128];
        DWORD readBytes;

        while (ReadFile(hOutRead, buf, sizeof(buf), &readBytes, nullptr) && readBytes > 0)
            stdoutStr.append(buf, readBytes);
        while (ReadFile(hErrRead, buf, sizeof(buf), &readBytes, nullptr) && readBytes > 0)
            stderrStr.append(buf, readBytes);

        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(hOutRead);
        CloseHandle(hErrRead);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        callback(ProcessResult{ (int)exitCode, stdoutStr, stderrStr });

#else
        // POSIX version
        int pipeOut[2], pipeErr[2];
        if (pipe(pipeOut) == -1 || pipe(pipeErr) == -1) { callback(std::nullopt); return; }

        pid_t pid = fork();
        if (pid < 0) { callback(std::nullopt); return; }
        if (pid == 0) {
            dup2(pipeOut[1], STDOUT_FILENO);
            dup2(pipeErr[1], STDERR_FILENO);
            close(pipeOut[0]); close(pipeOut[1]);
            close(pipeErr[0]); close(pipeErr[1]);
            execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
            _exit(127);
        }

        close(pipeOut[1]);
        close(pipeErr[1]);

        std::string stdoutStr, stderrStr;
        std::thread readStdout([&]() {
            char buf[128]; ssize_t n;
            while ((n = read(pipeOut[0], buf, sizeof(buf))) > 0) stdoutStr.append(buf, n);
        });
        std::thread readStderr([&]() {
            char buf[128]; ssize_t n;
            while ((n = read(pipeErr[0], buf, sizeof(buf))) > 0) stderrStr.append(buf, n);
        });

        int status = 0;
        bool timedOut = false;

        if (timeout.count() > 0) {
            auto start = std::chrono::steady_clock::now();
            while (true) {
                pid_t ret = waitpid(pid, &status, WNOHANG);
                if (ret == pid) break;
                auto elapsed = std::chrono::steady_clock::now() - start;
                if (elapsed >= timeout) {
                    kill(pid, SIGKILL);
                    timedOut = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        } else {
            waitpid(pid, &status, 0);
        }

        readStdout.join();
        readStderr.join();

        close(pipeOut[0]);
        close(pipeErr[0]);

        if (timedOut) { callback(std::nullopt); return; }

        callback(ProcessResult{
            WIFEXITED(status) ? WEXITSTATUS(status) : -1,
            stdoutStr, stderrStr
        });
#endif
    }).detach();
}
}//namespace rat::process
