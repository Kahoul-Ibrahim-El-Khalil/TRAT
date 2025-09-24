#include "DrogonRatServer/Handler.hpp"
#include "DrogonRatServer/debug.hpp"

#include <filesystem>
#include <string_view>

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

// --- Embed schema.sql here ---
constexpr std::string_view TELEGRAM_SCHEMA_SQL = R"sql(
CREATE TABLE IF NOT EXISTS telegram_bot (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    token TEXT UNIQUE NOT NULL,
    can_receive_updates BOOLEAN DEFAULT 1
);

CREATE TABLE IF NOT EXISTS telegram_message (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    bot_id INTEGER NOT NULL REFERENCES telegram_bot(id),
    text TEXT,
    caption TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS telegram_file (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    message_id INTEGER NOT NULL REFERENCES telegram_message(id) ON DELETE CASCADE,
    file_path TEXT NOT NULL,
    mime_type TEXT,
    extension TEXT,
    data BLOB NOT NULL
);

CREATE TABLE IF NOT EXISTS telegram_update (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    bot_id INTEGER NOT NULL REFERENCES telegram_bot(id),
    message_id INTEGER REFERENCES telegram_message(id),
    delivered BOOLEAN DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
)sql";

Handler::Handler(drogon::HttpAppFramework &Drogon_App)
    : drogon_app(Drogon_App) {
	INFO_LOG("Handler constructed");
}

Handler &Handler::setDbFilePath(const std::filesystem::path &Db_File_Path) {
	this->db_file_path = Db_File_Path;
	return *this;
}

Handler &Handler::initDbClient(void) {
	// Always create/open the SQLite file
	this->p_db_client =
	    drogon::orm::DbClient::newSqlite3Client(this->db_file_path.string(), 1);

	INFO_LOG("Instantiating SQLITE3 client from file {}",
	         this->db_file_path.string());

	    // Apply schema (synchronously for simplicity at startup)
		try {
			this->p_db_client->execSqlSync(std::string(TELEGRAM_SCHEMA_SQL));
			INFO_LOG("Database schema ensured at {}",
			         this->db_file_path.string());
		}
		catch(const std::exception &e) {
			ERROR_LOG("Failed to apply schema: {}", e.what());
		}

	DEBUG_SQLITE3_DB_FILE_CONTENT(db_file_path.string());

	return *this;
}
} // namespace DrogonRatServer
