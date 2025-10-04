/*server/src/Handler/constructor.cpp*/

#include "DrogonRatServer/Handler.hpp"
#include "DrogonRatServer/SQLSchemaStatements.hpp"
#include "DrogonRatServer/debug.hpp"

#include <filesystem>
#include <memory>

namespace DrogonRatServer {

inline void _applySqlSchema(const drogon::orm::DbClientPtr &Data_Base) {
    try {
        for(const auto &stmt : SCHEMA_TABLE_CREATION_STATEMENTS) {
            Data_Base->execSqlSync(std::string(stmt));
        }
    } catch(const std::exception &e) {
        FILE_ERROR_LOG("Failed to apply schema: {}", e.what());
    }
}

Handler::Handler(drogon::HttpAppFramework &Drogon_App) : drogon_app(Drogon_App) {
}

Handler &Handler::setDbFilePath(const std::filesystem::path &Db_File_Path) {
    this->db_file_path = Db_File_Path;
    return *this;
}

Handler &Handler::initDbClient() {
    try {
        // data_dowload_dir = std::filesystem::current_path() / "data/downloads"
        if(!std::filesystem::exists(this->data_dowload_dir)) {
            if(std::filesystem::create_directories(this->data_dowload_dir)) {
            } else {
                FILE_ERROR_LOG("{} failed to be created", this->data_dowload_dir.string());
            }
        }
        const std::filesystem::path &db_file_parent_dir = this->db_file_path.parent_path();
        if(!std::filesystem::exists(db_file_parent_dir)) {
            if(std::filesystem::create_directories(db_file_parent_dir)) {
            } else {
                FILE_ERROR_LOG("{} failed to be created", db_file_parent_dir.string());
            }
        }
        this->p_db_client = drogon::orm::DbClient::newSqlite3Client(
            fmt::format("filename={}", std::filesystem::absolute(this->db_file_path).string()),
            1);

        if(!this->p_db_client) {
            throw std::runtime_error("Failed to create SQLite3 client");
        }

        _applySqlSchema(this->p_db_client);
    } catch(const std::exception &e) {
        FILE_ERROR_LOG("Database init failed: {}", e.what());
    }

    try {
        auto result = p_db_client->execSqlSync("SELECT id, token FROM telegram_bot;");
        this->cached_bots.clear();
        for(auto const &row : result) {
            cached_bots.push_back({row["id"].as<int64_t>(), row["token"].as<std::string>()});
        }
    } catch(const std::exception &e) {
        FILE_ERROR_LOG("Failed to preload bots into cache: {}", e.what());
    }
    return *this;
}

Handler &Handler::initShell(const int64_t &Server_Id) {
    const int64_t &update_offset = this->queryUpdateOffset();
    const int64_t &message_offset = this->queryMessageOffset();
    this->shell_sptr =
        std::make_shared<DrogonRatServer::Handler::Shell>(update_offset, message_offset, Server_Id);
    return *this;
}
} // namespace DrogonRatServer
