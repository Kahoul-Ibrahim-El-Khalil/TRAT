/*rat_networking/rat/include/networking.hpp*/
#pragma once
#include "rat/networking/helpers.hpp"
#include "rat/networking/types.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <curl/curl.h>
#include <filesystem>
#include <string>
#include <vector>

namespace rat::networking {

struct EasyCurlHandler {
    CURL *curl;
    CURLcode state;

    EasyCurlHandler();
    ~EasyCurlHandler();

    // Type-safe setOption overloads
    CURLcode setOption(CURLoption Curl_Option, long value);
    CURLcode setOption(CURLoption Curl_Option, const char *value);
    CURLcode setOption(CURLoption Curl_Option, void *value);

    /*
    CURLcode setOption(CURLoption Curl_Option, curl_off_t value);
    since curl_off_t is a typedef of long, there is no need to overload it.
    */
    CURLcode setUrl(const std::string &arg_Url);
    CURLcode setUrl(const char *arg_Url);

    using WriteCallback = size_t (*)(void *, size_t, size_t, void *);
    CURLcode setWriteCallBackFunction(WriteCallback Call_Back);

    CURLcode perform();
    void resetOptions();
};

class Client : public EasyCurlHandler {
  private:
    uint8_t server_endpoints_count = 1;

    bool allow_self_signed_certs = false;

  public:
    void resetOptions();
    CURLcode reset();

    Client(uint8_t Typical_Server_Endpoints_count = 1)
        : server_endpoints_count(Typical_Server_Endpoints_count) {
        curl_easy_setopt(this->curl, CURLOPT_MAXCONNECTS, (long)(this->server_endpoints_count));
        curl_easy_setopt(this->curl, CURLOPT_MAXLIFETIME_CONN, 1L);
        curl_easy_setopt(this->curl, CURLOPT_FRESH_CONNECT,
                         1L); // New connection each time
    };

    Client &allowSelfSignedCerts() {
        this->allow_self_signed_certs = true;
        return *this;
    }

    NetworkingResult download(const char *Downloading_Url, const std::filesystem::path &File_Path);

    inline NetworkingResult
    download(const std::string &Downloading_Url, const std::filesystem::path &File_Path) {
        return this->download(Downloading_Url.c_str(), File_Path);
    }

    inline NetworkingResult download(const char *Downloading_Url) {
        auto path = std::filesystem::current_path() / _getFilePathFromUrl(Downloading_Url);
        return this->download(Downloading_Url, path);
    }

    inline NetworkingResult download(const std::string &Downloading_Url) {
        return this->download(Downloading_Url.c_str());
    }

    NetworkingResult upload(const std::filesystem::path &File_Path, const char *Uploading_Url);

    inline NetworkingResult
    upload(const std::filesystem::path &File_Path, const std::string &Uploading_Url) {
        return this->upload(File_Path, Uploading_Url.c_str());
    }

    NetworkingResult
    uploadMimeFile(const MimeContext &Mime_Context, std::vector<char> &Response_Buffer);

    NetworkingResult sendHttpRequest(const char *arg_Url, std::vector<char> &arg_Buffer);

    inline NetworkingResult
    sendHttpRequest(const std::string &arg_Url, std::vector<char> &arg_Buffer) {
        return this->sendHttpRequest(arg_Url.c_str(), arg_Buffer);
    }

    NetworkingResult sendHttpRequest(const char *arg_Url, char *p_Buffer, size_t Buffer_Size);

    inline NetworkingResult
    sendHttpRequest(const std::string &arg_Url, char *p_Buffer, size_t Buffer_Size) {
        return this->sendHttpRequest(arg_Url.c_str(), p_Buffer, Buffer_Size);
    }

    NetworkingResult downloadData(const std::string &arg_Url, std::vector<uint8_t> &Out_Buffer);
};

} // namespace rat::networking
