#pragma once
#include <string>
#include <optional>
#include <chrono>
#include <functional>

namespace rat::process {
struct ProcessResult {
    int exitCode;
    std::string stdoutStr;
    std::string stderrStr;
};

using ProcessCallback = std::function<void(std::optional<ProcessResult>)>;

void runAsyncProcess(
    const std::string& command,
    std::chrono::milliseconds timeout,
    ProcessCallback callback
);

}//namespace rat::process

