#include "rat/system.hpp"
namespace rat::system {

// -------------------------------
// Filesystem Navigation
// -------------------------------

// Change current working directory
std::string cd(const std::filesystem::path& arg_Path) {
    try {
        if (!std::filesystem::exists(arg_Path) || !isDir(arg_Path)) {
            return fmt::format("Error: Path '{}' does not exist or is not a directory.", arg_Path.string());
        }
        std::filesystem::current_path(arg_Path);
        return pwd();
    } catch (const std::filesystem::filesystem_error& e) {
        return fmt::format("Filesystem error: {}", e.what());
    } catch (const std::exception& e) {
        return fmt::format("Unknown error: {}", e.what());
    }
}

} // namespace rat::system

