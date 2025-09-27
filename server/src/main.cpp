#include "DrogonRatServer/Server.hpp"

#include <filesystem>

const std::filesystem::path &DB_FILE_PATH =
    std::filesystem::absolute(std::filesystem::current_path() / "data/data.db");

int main() {
    DrogonRatServer::Server server(drogon::app(), DB_FILE_PATH);
    server.setIps({"0.0.0.0", "192.168.1.6"}).run();
}
