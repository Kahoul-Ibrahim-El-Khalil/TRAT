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

namespace rat::networking {

std::filesystem::path _getFilePathFromUrl(const std::string& arg_Url);

struct BufferContext {
    char*  buffer;     // pointer to fixed-size buffer
    size_t capacity;   // total size available
    size_t size;       // number of bytes written
};

struct EasyCurlHandler {
    CURL*    curl;
    CURLcode state;

    EasyCurlHandler();                           // Constructor
    ~EasyCurlHandler();                          // Destructor

    // Type-safe setOption overloads
    CURLcode setOption(CURLoption Curl_Option, long value);
    CURLcode setOption(CURLoption Curl_Option, const char* value);
    CURLcode setOption(CURLoption Curl_Option, void* value);
    
    /*
    CURLcode setOption(CURLoption Curl_Option, curl_off_t value);
    since curl_off_t is a typedef of long, there is no need to overload it.
    */
    CURLcode setUrl(const std::string& arg_Url);

    using WriteCallback = size_t(*)(void*, size_t, size_t, void*);
    CURLcode setWriteCallBackFunction(WriteCallback Call_Back);

    CURLcode perform();                          // Perform the request
    void reset();
};

class Client : public EasyCurlHandler {          // Explicit public inheritance
private:
    std::chrono::milliseconds action_duration{0};

public:
    bool download(const std::string& Downloading_Url, const std::filesystem::path& File_Path);

    /* Overloaded: extracts the file name from url and downloads to the current directory */
    bool download(const std::string& Download_Url);

    bool upload(const std::filesystem::path& File_Path, const std::string& Uploading_Url);

    bool uploadMimeFile(const std::filesystem::path& File_Path,
                        const std::string& arg_Url,
                        const std::string& Field_Name,
                        const std::string& Mime_Type = "");

    std::vector<char> sendHttpRequest(const std::string& arg_Url);

    /* Returns the actual bytes written into p_Buffer (truncates if response > Buffer_Size) */
    size_t sendHttpRequest(const std::string& arg_Url, char* p_Buffer, size_t Buffer_Size);

    bool downloadData(const std::string& arg_Url, std::vector<uint8_t>& Out_Buffer);
};

} // namespace rat::networking

