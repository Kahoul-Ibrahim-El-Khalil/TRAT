#pragma once
#include <string>
#include <optional>
#include <chrono>
#include <functional>
#include "rat/ThreadPool.hpp"
#include <mutex>
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
    ::rat::threading::ThreadPool* thread_pool; //refrence for the thread pool
    std::optional<std::vector<std::mutex*>> mutexes; //for safety purposes;
    ProcessCallback callback;
};


void runAsyncProcess(ProcessContext& Process_Context);

}//namespace rat::process

