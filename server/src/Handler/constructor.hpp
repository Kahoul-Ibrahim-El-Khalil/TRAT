#pragma once
#ifdef DEBUG
#include <array>
#include <cstdio>
#include <memory>
#include <string>

#define DEBUG_SQLITE3_DB_FILE_CONTENT(db_path)                                 \
    do {                                                                       \
        std::array<char, 128> buffer{};                                        \
        std::string tables;                                                    \
        std::unique_ptr<FILE, decltype(&pclose)> pipe(                         \
            popen((std::string("sqlite3 ") + (db_path) + " \".tables\"")       \
                      .c_str(),                                                \
                  "r"),                                                        \
            pclose);                                                           \
        if(pipe) {                                                             \
            while(fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)  \
                tables += buffer.data();                                       \
            DEBUG_LOG("SQLite3 DB tables: {}", tables);                        \
        } else {                                                               \
            ERROR_LOG("Failed to open pipe for SQLite3 DB: {}", db_path);      \
        }                                                                      \
    } while(0)
#else
#define DEBUG_SQLITE3_DB_FILE_CONTENT(db_path)                                 \
    do {                                                                       \
    } while(0)
#endif
