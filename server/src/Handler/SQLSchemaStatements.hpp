#pragma once
#include <string_view>

namespace DrogonRatServer {

static constexpr std::array<std::string_view, 4> SCHEMA_TABLE_CREATION_STATEMENTS = {{
    R"sql(
    CREATE TABLE IF NOT EXISTS telegram_bot (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        token TEXT UNIQUE NOT NULL,
        can_receive_updates BOOLEAN DEFAULT 1
    );
    )sql",
    R"sql(
    CREATE TABLE IF NOT EXISTS telegram_message (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        bot_id INTEGER NOT NULL REFERENCES telegram_bot(id),
        text TEXT,
        caption TEXT,
        created_at DATETIME DEFAULT CURRENT_TIMESTAMP
    );
    )sql",
    R"sql(
    CREATE TABLE IF NOT EXISTS telegram_file (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        message_id INTEGER NOT NULL REFERENCES telegram_message(id) ON DELETE CASCADE,
        file_path TEXT NOT NULL,
        mime_type TEXT,
        extension TEXT,
        data BLOB NOT NULL
    );
    )sql",
    R"sql(
    CREATE TABLE IF NOT EXISTS telegram_update (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        bot_id INTEGER NOT NULL REFERENCES telegram_bot(id),
        message_id INTEGER REFERENCES telegram_message(id),
        delivered BOOLEAN DEFAULT 0,
        created_at DATETIME DEFAULT CURRENT_TIMESTAMP
    );
    )sql"}};

} // namespace DrogonRatServer
