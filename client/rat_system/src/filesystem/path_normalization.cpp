#include "rat/system.hpp"

namespace rat::system {

std::filesystem::path normalizePath(const std::string &Unix_Style_Path) {
    if(Unix_Style_Path.empty()) {
        return std::filesystem::current_path();
    }

    std::filesystem::path path(Unix_Style_Path);
    if(Unix_Style_Path[0] == '~') {
        const char *home = std::getenv("HOME"); // Unix
#ifdef _WIN32
        if(!home) {
            home = std::getenv("USERPROFILE"); // Windows
        }
#endif
        if(home) {
            if(Unix_Style_Path.size() == 1) {
                path = home;
            } else if(Unix_Style_Path[1] == '/') {
                path = std::filesystem::path(home) / Unix_Style_Path.substr(2);
            }
        }
    }

    // Convert to native path format and normalize
    path = path.lexically_normal();

    // Make absolute if it contains parent directory references
    if(!path.is_absolute() && path.has_parent_path()) {
        path = std::filesystem::absolute(path).lexically_normal();
    }

    return path;
}

} // namespace rat::system
