#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>
#include <functional>
#include <unordered_map>
#include <fmt/core.h>

#include "rat/system.hpp"  // your header

using namespace rat::system;

static void print_help() {
    fmt::print("Available commands:\n");
    fmt::print("  datetime      -> print current date/time\n");
    fmt::print("  user          -> print username\n");
    fmt::print("  os            -> print OS info\n");
    fmt::print("  cpu           -> print CPU info\n");
    fmt::print("  mem           -> print Memory info\n");
    fmt::print("  net           -> print Network info\n");
    fmt::print("  pwd           -> print current directory\n");
    fmt::print("  ls [path]     -> list directory contents\n");
    fmt::print("  read <file>   -> read file\n");
    fmt::print("  write <file> <text> -> overwrite file\n");
    fmt::print("  append <file> <text> -> append text to file\n");
    fmt::print("  rm <file>     -> remove file\n");
    fmt::print("  mkdir <path>  -> create directory\n");
    fmt::print("  rmdir <path>  -> remove directory\n");
    fmt::print("  ps            -> list processes\n");
    fmt::print("  pid           -> print this process ID\n");
    fmt::print("  run <cmd>     -> run command detached\n");
    fmt::print("  exit          -> quit shell\n");
    fmt::print("\n");
}

int main() {
    fmt::print("rat::system Debug Shell\nType 'help' for commands.\n\n");

    std::string line;
    while (true) {
        fmt::print("rat> ");
        if (!std::getline(std::cin, line)) break;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "exit" || cmd == "quit") break;
        if (cmd == "help") { print_help(); continue; }

        try {
            if (cmd == "datetime") {
                fmt::print("{}\n", getCurrentDateTime());
            } else if (cmd == "user") {
                fmt::print("{}\n", getUserName());
            } else if (cmd == "os") {
                fmt::print("{}\n", getOsInfo());
            } else if (cmd == "cpu") {
                fmt::print("{}\n", getCpuInfo());
            } else if (cmd == "mem") {
                fmt::print("{}\n", getMemoryInfo());
            } else if (cmd == "net") {
                fmt::print("{}\n", getNetworkInfo());
            } else if (cmd == "pwd") {
                fmt::print("{}\n", pwd());
            } else if (cmd == "ls") {
                std::string path; iss >> path;
                fmt::print("{}\n", ls(path.empty() ? "." : path));
            } else if (cmd == "read") {
                std::string file; iss >> file;
                fmt::print("{}\n", readFile(file));
            } else if (cmd == "write") {
                std::string file; iss >> file;
                std::string text; std::getline(iss, text);
                if (echo(text, file)) fmt::print("OK\n"); else fmt::print("FAIL\n");
            } else if (cmd == "append") {
                std::string file; iss >> file;
                std::string text; std::getline(iss, text);
                if (appendToFile(text, file)) fmt::print("OK\n"); else fmt::print("FAIL\n");
            } else if (cmd == "rm") {
                std::string file; iss >> file;
                if (removeFile(file)) fmt::print("Removed {}\n", file); else fmt::print("FAIL\n");
            } else if (cmd == "mkdir") {
                std::string dir; iss >> dir;
                if (createDir(dir)) fmt::print("Created {}\n", dir); else fmt::print("FAIL\n");
            } else if (cmd == "rmdir") {
                std::string dir; iss >> dir;
                if (removeDir(dir)) fmt::print("Removed {}\n", dir); else fmt::print("FAIL\n");
            } else if (cmd == "ps") {
                fmt::print("{}\n", listProcesses());
            } else if (cmd == "pid") {
                fmt::print("PID: {}\n", getProcessId());
            } else if (cmd == "run") {
                //runCommandOnSeparateThread(task);
                fmt::print("Not implimented yet\n");
          
            } else {
                fmt::print("Unknown command '{}'. Type 'help'.\n", cmd);
            }
        } catch (const std::exception& e) {
            fmt::print(stderr, "Error: {}\n", e.what());
        }
    }

    fmt::print("Goodbye.\n");
    return 0;
}
