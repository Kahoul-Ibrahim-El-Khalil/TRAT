/*rat_networking/rat/include/networking.hpp*/
#pragma once

#include <cstring>
#include <string>
#include <array>
#include <vector>
#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <cstdint>
#include <curl/curl.h>
#include <map>
#include "rat/networking/types.hpp"

#include "rat/networking/helpers.hpp"

namespace rat::networking {

struct EasyCurlHandler {
    CURL*    curl;
    CURLcode state;

    EasyCurlHandler();                           
    ~EasyCurlHandler();                          

    // Type-safe setOption overloads
    CURLcode setOption(CURLoption Curl_Option, long value);
    CURLcode setOption(CURLoption Curl_Option, const char* value);
    CURLcode setOption(CURLoption Curl_Option, void* value);
    
    /*
    CURLcode setOption(CURLoption Curl_Option, curl_off_t value);
    since curl_off_t is a typedef of long, there is no need to overload it.
    */
    CURLcode setUrl(const std::string& arg_Url);
    CURLcode setUrl(const char* arg_Url);

    using WriteCallback = size_t(*)(void*, size_t, size_t, void*);
    CURLcode setWriteCallBackFunction(WriteCallback Call_Back);

    CURLcode perform();                          

    void resetOptions(); 
    /*Destroys the handle, clean it and re-init it*/
    bool hardResetHandle();

};


class Client : public EasyCurlHandler {          
private:
    uint8_t post_restart_operation_count = 0;
    uint8_t operation_restart_bound;
    uint8_t server_endpoints_count = 1;
    bool is_fresh;
    void reset();

public:
    Client(uint8_t Operation_Restart_Bound = 5, uint8_t Typical_Server_Endpoints_count = 1) 
    :   operation_restart_bound(Operation_Restart_Bound),
        server_endpoints_count(Typical_Server_Endpoints_count),
        is_fresh(true)
    {
        if (this->operation_restart_bound == 0) {
            this->operation_restart_bound = 1; // Ensure at least 1 operation
        }
        curl_easy_setopt(this->curl, CURLOPT_MAXCONNECTS, (long)(this->server_endpoints_count));
        curl_easy_setopt(this->curl, CURLOPT_MAXLIFETIME_CONN , 1L);
        curl_easy_setopt(this->curl, CURLOPT_FRESH_CONNECT, 1L);         // New connection each time

    };
    
    bool download(const char* Downloading_Url, const std::filesystem::path& File_Path);
    
    inline bool download(const std::string& Downloading_Url, const std::filesystem::path& File_Path) {
        return this->download(Downloading_Url.c_str());    
    }

    inline bool download(const char* Downloading_Url) {
        auto path = std::filesystem::current_path() / _getFilePathFromUrl(Downloading_Url);
        return this->download(Downloading_Url, path);
    }
    inline bool download(const std::string& Downloading_Url) {
        return this->download(Downloading_Url.c_str());
    }

    bool upload(const std::filesystem::path& File_Path, const char* Uploading_Url);
    inline bool upload(const std::filesystem::path& File_Path, const std::string& Uploading_Url) {
        return this->upload(File_Path, Uploading_Url.c_str());
    }
    bool uploadMimeFile(const MimeContext& Mime_Context);

    std::vector<char> sendHttpRequest(const char* arg_Url);
    inline std::vector<char> sendHttpRequest(const std::string& arg_Url) {
        return this->sendHttpRequest(arg_Url.c_str());   
    }
    /* Returns the actual bytes written into p_Buffer (truncates if response > Buffer_Size) */
    
    size_t sendHttpRequest(const char* arg_Url, char* p_Buffer, size_t Buffer_Size);
    inline size_t sendHttpRequest(const std::string& arg_Url, char* p_Buffer, size_t Buffer_Size) {
        return this->sendHttpRequest(arg_Url.c_str(), p_Buffer, Buffer_Size);
    }

    bool downloadData(const std::string& arg_Url, std::vector<uint8_t>& Out_Buffer);
};


} // namespace rat::networking

