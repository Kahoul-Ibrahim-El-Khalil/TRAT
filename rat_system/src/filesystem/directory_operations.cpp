#include "rat/system.hpp"
namespace rat::system {

bool createDir(const std::filesystem::path& Dir_Path) {
    try {
        if (std::filesystem::exists(Dir_Path)) return false;
        return std::filesystem::create_directory(Dir_Path);
    } catch (...) {
        return false;
    }
}

bool removeDir(const std::filesystem::path& Dir_Path) {
    try {
        if (!std::filesystem::exists(Dir_Path) || !isDir(Dir_Path)) return false;
        std::filesystem::remove_all(Dir_Path);
        return true;
    } catch (...) {
        return false;
    }
}


bool moveDir(const std::filesystem::path& Original_Path, const std::filesystem::path& Target_Path) {
    try {
        if (!std::filesystem::exists(Original_Path) || !isDir(Original_Path)) return false;
        std::filesystem::rename(Original_Path, Target_Path);
        return true;
    } catch (...) {
        return false;
    }
}

size_t getDirSize(const std::filesystem::path& Dir_Path) {
    try {
        if (!std::filesystem::exists(Dir_Path) || !isDir(Dir_Path)) return 0;

        size_t totalSize = 0;
        for (auto& entry : std::filesystem::recursive_directory_iterator(Dir_Path)) {
            if (isFile(entry.path())) {
                totalSize += getFileSize(entry.path());
            }
        }
        return totalSize;
    } catch (...) {
        return 0;
    }
}
bool copyDir(const std::filesystem::path& Original_Dir, const std::filesystem::path& Target_Dir) {
    try {
        if (std::filesystem::exists(Original_Dir)) {
            return false;
        }
        if (!std::filesystem::is_directory(Original_Dir)) {
            return false;
        }

        if (!std::filesystem::exists(Target_Dir)) {
            if (!std::filesystem::create_directories(Target_Dir)) {
                return false;
            }
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(Original_Dir)) {
            try {
                const auto& path = entry.path();
                auto relative_path = std::filesystem::relative(path, Original_Dir);
                auto target_path = Target_Dir / relative_path;

                if (std::filesystem::is_directory(path)) {
                    if (!std::filesystem::exists(target_path) && !std::filesystem::create_directory(target_path)) {
                        return false;
                    }
                } else if (std::filesystem::is_regular_file(path)) {
                    std::filesystem::copy_file(path, target_path, std::filesystem::copy_options::overwrite_existing);
                }
                // Skip other types (symlinks, etc.)
            } catch (const std::filesystem::filesystem_error& e) {
                return false;
            }
        }
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        return false;
    } catch (const std::exception& e) {
        return false;
    }
}
} // namespace rat::system

