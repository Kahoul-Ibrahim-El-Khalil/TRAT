/*TRAT/server/include/rat/server/Server.hpp*/
#pragma once
#include <drogon/drogon.h>
#include <string_view>

namespace rat::server {
class Server {
  private:
	std::string_view json_config_file;

  public:
	drogon::server drogon_server;

	&Server init(void);
	&Server run(void);
}
} // namespace rat::server
