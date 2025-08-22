#include "rat/system.hpp"
namespace rat::system {

// -------------------------------
// Basic Queries
// -------------------------------

bool exists(const std::filesystem::path& arg_Path) {
    return std::filesystem::exists(arg_Path);
}

bool isFile(const std::filesystem::path& arg_Path) {
    return std::filesystem::is_regular_file(arg_Path);
}

bool isDir(const std::filesystem::path& arg_Path) {
    return std::filesystem::is_directory(arg_Path);
}

std::string pwd(void) {
    return std::filesystem::current_path().string();
}

// List directory contents (files and subdirs)
std::string ls(const std::filesystem::path& arg_Path) {
    if (!std::filesystem::exists(arg_Path) || !isDir(arg_Path)) return {};

    std::ostringstream oss;
    for (auto& entry : std::filesystem::directory_iterator(arg_Path)) {
        oss << entry.path().filename().string();
        if (isDir(entry.path())) oss << "/";
        oss << "\n";
    }
    return oss.str();
}

// Recursive tree listing
std::string tree(const std::filesystem::path& Dir_Path, const std::string& prefix = "") {
    if (!std::filesystem::exists(Dir_Path) || !isDir(Dir_Path)) return {};

    std::ostringstream oss;
    for (auto& entry : std::filesystem::directory_iterator(Dir_Path)) {
        oss << prefix << entry.path().filename().string();
        if (isDir(entry.path())) {
            oss << "/\n" << tree(entry.path(), prefix + "  ");
        } else {
            oss << "\n";
        }
    }
    return oss.str();
}

} // namespace rat::system

