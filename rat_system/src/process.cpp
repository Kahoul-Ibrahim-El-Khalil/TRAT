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

#include "logging.hpp"
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
std::future<std::string> runShellCommand(const std::string& arg_Command,
                                         unsigned int Timeout_ms,
                                         rat::ThreadPool& Timer_Pool)
{
    return Timer_Pool.enqueue([arg_Command, Timeout_ms]() -> std::string {
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
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return fmt::to_string(buffer);
    });
}

std::future<rat::system::ProcessResult> runProcessAsync(
    const std::string& arg_Command,
    unsigned int Timeout_ms)
{
    auto stdout_buffer = std::make_shared<std::ostringstream>();
    auto stderr_buffer = std::make_shared<std::ostringstream>();

    auto process_ptr = std::make_shared<TinyProcessLib::Process>(
        arg_Command,
        "",
        [stdout_buffer](const char* bytes, size_t n){ stdout_buffer->write(bytes, n); },
        [stderr_buffer](const char* bytes, size_t n){ stderr_buffer->write(bytes, n); }
    );

    return std::async(std::launch::async, [process_ptr, stdout_buffer, stderr_buffer, Timeout_ms]() {
        rat::system::ProcessResult result;
        try {
            int exit_code = -1;

            if (Timeout_ms == 0) {
                // Detached mode: process keeps running, we don’t wait for it
                result.exit_code  = 0; // convention: detached success
                result.stdout_str = "";
                result.stderr_str = "";
                return result;
            }

            // Timed execution mode
            auto start = std::chrono::steady_clock::now();
            while (true) {
                if (process_ptr->try_get_exit_status(exit_code)) break;
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > Timeout_ms) {
                    process_ptr->kill();
                    exit_code = -1; // killed due to timeout
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            result.exit_code  = exit_code;
            result.stdout_str = stdout_buffer->str();
            result.stderr_str = stderr_buffer->str();

        } catch (const std::exception& ex) {
            result.exit_code  = -2;
            result.stdout_str = "";
            result.stderr_str = fmt::format("Exception: {}", ex.what());
        }
        return result;
    });
}
std::future<rat::system::ProcessResult> runProcessAsync(
    const std::string& arg_Command,
    unsigned int Timeout_ms,
    rat::ThreadPool& Timer_Pool)
{
    auto stdout_buffer = std::make_shared<std::ostringstream>();
    auto stderr_buffer = std::make_shared<std::ostringstream>();

/*Asynchronous behavior*/
    auto process_ptr = std::make_shared<TinyProcessLib::Process>(
        arg_Command,
        "",
        [stdout_buffer](const char* bytes, size_t n){ stdout_buffer->write(bytes, n); },
        [stderr_buffer](const char* bytes, size_t n){ stderr_buffer->write(bytes, n); }
    );

    return Timer_Pool.enqueue([process_ptr, stdout_buffer, stderr_buffer, Timeout_ms]() -> rat::system::ProcessResult {
        rat::system::ProcessResult result;
        try {
            int exit_code = -1;

            if (Timeout_ms == 0) {
                result.exit_code  = 0; // detached
                result.stdout_str = "";
                result.stderr_str = "";
                return result;
            }

            auto start = std::chrono::steady_clock::now();
            while (true) {
                if (process_ptr->try_get_exit_status(exit_code)) break;
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > Timeout_ms) {
                    process_ptr->kill();
                    exit_code = -1; // killed due to timeout
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            result.exit_code  = exit_code;
            result.stdout_str = stdout_buffer->str();
            result.stderr_str = stderr_buffer->str();

        } catch (const std::exception& ex) {
            result.exit_code  = -2;
            result.stdout_str = "";
            result.stderr_str = fmt::format("Exception: {}", ex.what());
        }
        return result;
    });
}

}//namespace rat::system
