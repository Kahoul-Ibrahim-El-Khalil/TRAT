#include "DrogonRatServer/Server.hpp"

#include <filesystem>

const std::filesystem::path &DB_FILE_PATH = std::filesystem::absolute(
    std::filesystem::current_path() / "../TelegramBotApiModels/data.db");

using namespace DrogonRatServer;

int main() {
	Server server(drogon::app(), DB_FILE_PATH);
	server.setIps({"0.0.0.0", "192.168.1.6"}).run();
}
