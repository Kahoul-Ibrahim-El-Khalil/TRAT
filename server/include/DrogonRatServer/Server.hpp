/*include/DrogonRatServer/Server.hpp*/
#pragma once
#include "DrogonRatServer/Handler.hpp"

#include <drogon/HttpAppFramework.h>
#include <drogon/drogon.h>
#include <filesystem>
#include <string>

namespace DrogonRatServer {

class Server {
  private:
    Handler handler;
    std::vector<std::string> ips;
    const std::string key_path = "key.key";
    const std::string certificate_path = "certificate.cert";

  public:
    Server(const Server &Other_Server) = delete;
    Server(Server &&Other_Server) = delete;

    explicit Server(drogon::HttpAppFramework &Drogon_Singelton,
                    const std::filesystem::path &Db_File_Path,
                    const int64_t &Server_id);
    Server &setIps(std::vector<std::string> &&arg_Ips);

    void run();
    ~Server();

  private:
    void generateSelfSignedCert(const std::string &Cert_Path, const std::string &Key_Path);
};

} // namespace DrogonRatServer
