#include "DrogonRatServer/Server.hpp"
#include "DrogonRatServer/debug.hpp"

namespace DrogonRatServer {

Server::Server(drogon::HttpAppFramework &Drogon_Singelton,
               const std::filesystem::path &Db_File_Path)
    : handler(Drogon_Singelton) {
    INFO_LOG("Setting Handler's Database file Path to {}",
             Db_File_Path.string());
    this->handler.setDbFilePath(Db_File_Path);
    INFO_LOG("Initializing database client");
    this->handler.initDbClient().registerAll();
}

} // namespace DrogonRatServer
