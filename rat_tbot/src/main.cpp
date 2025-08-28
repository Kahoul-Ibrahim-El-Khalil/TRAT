#include "logging.hpp"
#include "rat/networking.hpp"
#include <chrono>
#include <string>
#include <thread>
#include <fmt/core.h>
#include <iostream>
#include "nlohmann/json.hpp"

#define TELEGRAM_BOT_API_BASE_URL "https://api.telegram.org/bot"

const std::string TOKEN = "5598026972:AAFtLSUld27ImilkcOppvGMFgSAPXozEgVo"; 
constexpr int64_t MASTER_ID = 1492536442; 
static int message_id = 0;
static int update_id = 0;
void test_json_only(int iterations = 1) {
    // Use a real captured Telegram response
    const char* sample_response = R"({
        "update_id": 100,
        "message": {
            "message_id": 1,
            "text": "test message",
            "from": {"id": 123, "first_name": "Test"},
            "chat": {"id": 456, "type": "private"}
        }
    })";
    
    for (int i = 0; i < iterations; i++) {
        // Just parse, don't do anything with result
        auto json = nlohmann::json::parse(sample_response);
        
        // Simulate some basic extraction (like your real code)
        update_id = json.value("update_id", 0);
        if (json.contains("message")) {
            auto& msg = json["message"];
            std::string text = msg.value("text", "");
            message_id = msg.value("message_id", 0);
        }
    }
}

int main(void) {
    ::rat::networking::Client client(5, 1);
    
    constexpr size_t BUFFER_SIZE = 16 * 1024;
    std::vector<char> buffer(BUFFER_SIZE);
    
    std::string update_url = fmt::format("{}{}/getUpdates?timeout=1&limit=1&offset=0", 
                                       TELEGRAM_BOT_API_BASE_URL, TOKEN);
    
    std::string message_url = fmt::format("{}{}/sendMessage?chat_id={}&text=test", 
                                         TELEGRAM_BOT_API_BASE_URL, TOKEN, MASTER_ID);

    DEBUG_LOG("Starting combined memory test");
    DEBUG_LOG("Monitoring: 1 network call + 1 JSON parse per iteration");
    
    int iteration = 0;
    while (true) {
        DEBUG_LOG("Iteration {}", iteration++);
        
        // 1. Network operation
        size_t bytes_written = client.sendHttpRequest(update_url.c_str(), buffer.data(), BUFFER_SIZE);
        std::cout << "Response: " << std::string(buffer.data(), bytes_written) << "\n";
        
        // 2. JSON parsing (ONLY 1 parse per iteration)
        test_json_only(10000);  // ← Only 1 parse, not 10,000!
        
        // 3. Send response
        {
            client.sendHttpRequest(message_url.c_str(), buffer.data(), BUFFER_SIZE);
        
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // 2 second delay
    }
    
    return 0;
}