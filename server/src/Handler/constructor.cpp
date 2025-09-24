#include "DrogonRatServer/Handler.hpp"
#include "DrogonRatServer/debug.hpp"

#include <filesystem>

#ifdef DEBUG
#include <array>
#include <cstdio>
#include <memory>
#include <string>

#define DEBUG_SQLITE3_DB_FILE_CONTENT(db_path)                                 \
		do {                                                                   \
			std::array<char, 128> buffer{};                                    \
			std::string tables;                                                \
			std::unique_ptr<FILE, decltype(&pclose)> pipe(                     \
			    popen((std::string("sqlite3 ") + (db_path) + " \".tables\"")   \
			              .c_str(),                                            \
			          "r"),                                                    \
			    pclose);                                                       \
				if(pipe) {                                                     \
						while(fgets(buffer.data(), buffer.size(),              \
						            pipe.get()) != nullptr) {                  \
							tables += buffer.data();                           \
						}                                                      \
					DEBUG_LOG("SQLite3 DB tables: {}", tables);                \
				}                                                              \
				else {                                                         \
					ERROR_LOG("Failed to open pipe for SQLite3 DB: {}",        \
					          db_path);                                        \
				}                                                              \
	} while(0)
#else
#define DEBUG_SQLITE3_DB_FILE_CONTENT(db_path)                                 \
		do {                                                                   \
	} while(0)
#endif

namespace DrogonRatServer {

/* Get a reference of the drogon app singleton from the scope of the server,
   which in return gets it from the scope of main.
   Register the routes from the map and call the callback that is the value.
*/

Handler::Handler(drogon::HttpAppFramework &Drogon_App)
    : drogon_app(Drogon_App) {
	INFO_LOG("Handler constructed");
}

Handler &Handler::setDbFilePath(const std::filesystem::path &Db_File_Path) {
	this->db_file_path = Db_File_Path;
	return *this;
}

Handler &Handler::initDbClient(void) {
		if(std::filesystem::exists(this->db_file_path)) {
			this->p_db_client = drogon::orm::DbClient::newSqlite3Client(
			    this->db_file_path.string(), // path to your SQLite file
			    1); // max connection pool size (SQLite iis file-based)
			INFO_LOG("Instantiating SQLITE3 client from file {}",
			         this->db_file_path.string());

			DEBUG_SQLITE3_DB_FILE_CONTENT(db_file_path.string());
			return *this;
		}
	ERROR_LOG("Failed to start since database file does not exist");
	return *this;
}

Handler &Handler::registerAll() {
	this->registerEchoHandler();
	this->registerUploadHandler();
	this->registerTelegramBotApiHandler();
	return *this;
}
} // namespace DrogonRatServer
