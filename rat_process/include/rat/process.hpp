#pragma once
#include <string>
#include <optional>
#include <chrono>
#include <functional>
#include "rat/ThreadPool.hpp"
#include <mutex>
#include <atomic>

namespace rat::process {

struct ProcessResult {
    int exit_code;
    std::string stdout_str;
    std::string stderr_str;
};

using ProcessCallback = std::function<void(std::optional<ProcessResult>)>;


struct ProcessContext {
    std::string command;
    std::chrono::milliseconds timeout;
    ::rat::threading::ThreadPool* thread_pool; //refrence for the thread pool that the process should enqueue in.
    std::optional<std::vector<std::mutex*>> mutexes; //for cross thread safety purposes;
    ::rat::threading::ThreadPool* secondary_thread_pool = nullptr; // at least of size two to run reading stdout and stderr;
    ProcessCallback callback;
};


void runAsyncProcess(ProcessContext& Process_Context, std::atomic<pid_t>& Process_Id);

}//namespace rat::process

