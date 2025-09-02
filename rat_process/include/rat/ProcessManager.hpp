#pragma once
#include <chrono>
#include <functional>
#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include "rat/ThreadPool.hpp"

#ifdef _WIN32
#include <windows.h>
using pid_t = DWORD;
#else
#include <sys/types.h>
#endif

namespace rat::process {

class ProcessManager {
private:
    std::string command;
    ::rat::threading::ThreadPool* p_secondary_threadpool = nullptr;  
    
    std::function<void(pid_t)> creation_callback;
    std::function<void(int, const std::string&, const std::string&)> exiting_callback;
    
    // Removed atomic - these are now simple member variables
    pid_t process_id = 0;
    int exit_code = -1;
    bool timed_out = false;
    
    std::chrono::milliseconds timeout = std::chrono::milliseconds(0);
    
    // Platform-specific implementation
    bool executeInternal();

public:
    std::string stdout_str;
    std::string stderr_str;

    ProcessManager() = default;
    ~ProcessManager();
    
    // Non-copyable but movable
    ProcessManager(const ProcessManager&) = delete;
    ProcessManager& operator=(const ProcessManager&) = delete;
    ProcessManager(ProcessManager&&) = default;
    ProcessManager& operator=(ProcessManager&&) = default;
    
    // Getters
    pid_t getProcessId() const { return process_id; }
    int getExitCode() const { return exit_code; }
    const std::string& getStdout() const { return stdout_str; }
    const std::string& getStderr() const { return stderr_str; }
    bool wasTimedOut() const { return timed_out; }
    bool wasSuccessful() const { return exit_code == 0 && !timed_out; }
    
    // Setters
    void setCommand(const std::string& arg_Command);
    void setCommand(std::string&& arg_Command);
    void setSecondaryThreadPool(::rat::threading::ThreadPool* p_Secondary_Thread_Pool);
    void setTimeout(std::chrono::milliseconds arg_Timeout);
    
    void setProcessCreationCallback(std::function<void(pid_t)>&& Creation_Callback);
    void execute();
};

} // namespace rat::process
