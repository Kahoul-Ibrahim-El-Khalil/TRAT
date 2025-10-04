#include "DrogonRatServer/Server.hpp"
#include "DrogonRatServer/debug.hpp"

namespace DrogonRatServer {

Server::Server(drogon::HttpAppFramework &Drogon_Singelton,
               const std::filesystem::path &Db_File_Path,
               const int64_t &Server_Id)
    : handler(Drogon_Singelton) {
    this->handler.setDbFilePath(Db_File_Path);
    this->handler.initDbClient().initShell(Server_Id).registerAll();
    this->handler.getShell()->interact();
}

} // namespace DrogonRatServer
