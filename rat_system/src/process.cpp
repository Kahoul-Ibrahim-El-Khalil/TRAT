#include "tiny-process-library/process.hpp"
#include "rat/system.hpp"

#include <thread>
#include <future>
#include <sstream>
#include <cstdlib> // for std::system
#include <memory>
#include <stdexcept>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cstdio>

namespace rat::system {

std::string runShellCommand(const std::string& arg_Command, unsigned int Timeout_ms) {
    std::atomic<bool> finished{false};
    fmt::memory_buffer buffer;

    std::thread runner([&]() {
        FILE* pipe = popen(arg_Command.c_str(), "r");
        if (!pipe) {
            fmt::format_to(std::back_inserter(buffer), "Failed to run command\n");
            finished = true;
            return;
        }
        char chunk[256];
        while (fgets(chunk, sizeof(chunk), pipe) != nullptr) {
            fmt::format_to(std::back_inserter(buffer), "{}", chunk);
        }
        pclose(pipe);
        finished = true;
    });

    auto start = std::chrono::steady_clock::now();
    while (!finished) {
        if (Timeout_ms > 0 &&
            std::chrono::steady_clock::now() - start > std::chrono::milliseconds(Timeout_ms)) {
            fmt::format_to(std::back_inserter(buffer), "Command timed out\n");
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (runner.joinable()) runner.join();

    return fmt::to_string(buffer);
}

std::string runProcess(
    const std::string& arg_Command, 
    unsigned int Timeout_ms, 
    std::function<void(ProcessResult)> lambda_Callback) {
    
    try {
        // Buffers for capturing stdout & stderr
        auto stdout_buffer = std::make_shared<std::ostringstream>();
        auto stderr_buffer = std::make_shared<std::ostringstream>();

        // Protects against race when process finishes while thread still running
        auto process_ptr = std::make_shared<TinyProcessLib::Process>(
            arg_Command,
            "",
            [stdout_buffer](const char *bytes, size_t n) {
                stdout_buffer->write(bytes, n);
            },
            [stderr_buffer](const char *bytes, size_t n) {
                stderr_buffer->write(bytes, n);
            }
        );

        std::thread([process_ptr, stdout_buffer, stderr_buffer, Timeout_ms, lambda_Callback]() {
            int exit_code = -1;

            if (Timeout_ms > 0) {
                auto start = std::chrono::steady_clock::now();
                while (true) {
                    if (process_ptr->try_get_exit_status(exit_code)) {
                        break; // finished
                    }
              
                    auto now = std::chrono::steady_clock::now();
                    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > Timeout_ms) {
                        process_ptr->kill();
                        exit_code = -1;
                        break;
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // small sleep to avoid busy spin
              }
            } else {
            exit_code = process_ptr->get_exit_status(); // wait indefinitely
      }

    // Build result
    ProcessResult result;
    result.exit_code   = exit_code;
    result.stdout_str  = stdout_buffer->str();
    result.stderr_str  = stderr_buffer->str();

    // Invoke callback
    lambda_Callback(result);
      }).detach();        return "Process launched successfully";
    } catch (const std::exception &ex) {
        return fmt::format("Failed to start process: {}" , ex.what() );
    }
}
}//namespace rat::system
