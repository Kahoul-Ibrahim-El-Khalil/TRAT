#include "rat/networking.hpp"
#include <cstring>
#include <iostream>
#include <string>
#include <filesystem>

using rat::networking::downloadFile;
using rat::networking::uploadFile;
using rat::networking::_getFilePathFromUrl;
using rat::networking::NetworkingResponse;
using rat::networking::NetworkingResponseStatus;

/*
 * Commands:
 *   simple_download <url>
 *   download <url> <file>
 *   upload <url> <file>
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage:\n"
                  << "  simple_download <url>\n"
                  << "  download <url> <file>\n"
                  << "  upload <url> <file>\n";
        return 1;
    }

    std::string command = argv[1];

    if ((argc == 3) && (command == "simple_download")) {
        const std::string url = argv[2];
        std::filesystem::path file_path = _getFilePathFromUrl(url);

        NetworkingResponse response = downloadFile(url, file_path);
        if (response.status == NetworkingResponseStatus::SUCCESS) {
            std::cout << "Download SUCCESS → " << file_path
                      << " (" << response.duration << " ms)\n";
        } else {
            std::cerr << "Download FAILED\n";
        }
    }
    else if ((argc == 4) && (command == "download")) {
        const std::string url = argv[2];
        const std::filesystem::path file_path = argv[3];

        NetworkingResponse response = downloadFile(url, file_path);
        if (response.status == NetworkingResponseStatus::SUCCESS) {
            std::cout << "Download SUCCESS → " << file_path
                      << " (" << response.duration << " ms)\n";
        } else {
            std::cerr << "Download FAILED\n";
        }
    }
    else if ((argc == 4) && (command == "upload")) {
        const std::string url = argv[2];
        const std::filesystem::path file_path = argv[3];

        NetworkingResponse response = uploadFile(file_path, url);
        if (response.status == NetworkingResponseStatus::SUCCESS) {
            std::cout << "Upload SUCCESS from " << file_path
                      << " (" << response.duration << " ms)\n";
        } else {
            std::cerr << "Upload FAILED\n";
        }
    }
    else {
        std::cerr << "Invalid arguments. Run without args to see usage.\n";
        return 1;
    }

    return 0;
}
