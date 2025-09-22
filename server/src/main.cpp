#include "DrogonRatServer/Server.hpp"

using namespace DrogonRatServer;

int main() {
  Server server(drogon::app());

  server.setIps({"0.0.0.0", "192.168.1.6"}).run();
}
