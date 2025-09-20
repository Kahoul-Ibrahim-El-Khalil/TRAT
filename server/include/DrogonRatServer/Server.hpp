/*include/DrogonRatServer/Server.hpp*/
#pragma once
#include "DrogonRatServer/Handler.hpp"
#include <cstdint>
#include <drogon/drogon.h>
#include <string>

namespace DrogonRatServer {

class Server {
private:
  std::vector<std::string> ips;
  Handler handler;
  std::string key_path = "key.key";
  std::string certificate_path = "certificate.cert";

public:
  Server() = default;

  Server &setIps(const std::vector<std::string> &arg_Ips);

  void run();
  ~Server();

private:
  void generateSelfSignedCert(const std::string &Cert_Path,
                              const std::string &Key_Path);
};

} // namespace DrogonRatServer
