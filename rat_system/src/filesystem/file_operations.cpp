#include "rat/system.hpp"
#include <vector>
namespace rat::system {

bool createFile(const std::filesystem::path& File_Path) {
    try {
        if (std::filesystem::exists(File_Path)) return false;
        std::ofstream ofs(File_Path);
        return ofs.good();
    } catch (...) {
        return false;
    }
}

std::string readFile(const std::filesystem::path& File_Path) {
    try {
        if (!std::filesystem::exists(File_Path) || !isFile(File_Path)) return {};
        std::ifstream ifs(File_Path, std::ios::binary);
        std::ostringstream oss;
        oss << ifs.rdbuf();
        return oss.str();
    } catch (...) {
        return {};
    }
}

bool readBytesFromFile(const std::filesystem::path& File_Path, std::vector<uint8_t>& arg_Buffer) {
    std::ifstream file(File_Path, std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size < 0) {
        return false;
    }

    arg_Buffer.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(arg_Buffer.data()), size)) {
        arg_Buffer.clear();
        return false;
    }
    return true;
}

bool writeBytesIntoFile(const std::filesystem::path& File_Path, const std::vector<uint8_t>& arg_Buffer) {
    std::ofstream file(File_Path, std::ios::binary);
    if (!file) {
        return false;
    }

    if (!arg_Buffer.empty()) {
        file.write(reinterpret_cast<const char*>(arg_Buffer.data()), arg_Buffer.size());
        if (!file) {
            return false;
        }
    }

    return true;
}
// Overwrite file content
bool echo(const std::string& arg_Buffer, const std::filesystem::path& File_Path) {
    try {
        std::ofstream ofs(File_Path, std::ios::binary | std::ios::trunc);
        if (!ofs.is_open()) return false;
        ofs << arg_Buffer;
        return true;
    } catch (...) {
        return false;
    }
}
bool echo(const std::vector<std::string>& arg_Buffers, const std::filesystem::path& File_Path) {
    try {
        std::ofstream ofs(File_Path, std::ios::binary | std::ios::trunc);
        if (!ofs.is_open()) return false;
        for(const std::string& buffer: arg_Buffers) ofs << buffer << '\n';
        return true;
    } catch (...) {
        return false;
    }
}

bool appendToFile(const std::string& arg_Buffer, const std::filesystem::path& File_Path) {
    try {
        std::ofstream ofs(File_Path, std::ios::binary | std::ios::app);
        if (!ofs.is_open()) return false;
        ofs << arg_Buffer;
        return true;
    } catch (...) {
        return false;
    }
}

bool removeFile(const std::filesystem::path& File_Path) {
    try {
        if (!std::filesystem::exists(File_Path) || !isFile(File_Path)) return false;
        return std::filesystem::remove(File_Path);
    } catch (...) {
        return false;
    }
}

size_t getFileSize(const std::filesystem::path& File_Path) {
    try {
        if (!rat::system::exists(File_Path) || !isFile(File_Path)) return 0;
        return static_cast<size_t>(std::filesystem::file_size(File_Path));
    } catch (...) {
        return 0;
    }
}

bool copyFile(const std::filesystem::path& Origin_Path, const std::filesystem::path& Target_Path) {
    try {
        if (!std::filesystem::exists(Origin_Path) || !isFile(Origin_Path)) return false;
        std::filesystem::copy_file(Origin_Path, Target_Path, std::filesystem::copy_options::overwrite_existing);
        return true;
    } catch (...) {
        return false;
    }
}

bool renameFile(const std::filesystem::path& Original_Path, const std::filesystem::path& Target_Path) {
    try {
        if (!std::filesystem::exists(Original_Path) || !isFile(Original_Path)) return false;
        std::filesystem::rename(Original_Path, Target_Path);
        return true;
    } catch (...) {
        return false;
    }
}
} // namespace rat::system

