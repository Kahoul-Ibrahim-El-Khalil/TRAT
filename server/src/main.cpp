#include "DrogonRatServer/Server.hpp"

using namespace DrogonRatServer;

int main() {
  Server server;

  server.setIps({"0.0.0.0", "192.168.1.6"}).run();
}
