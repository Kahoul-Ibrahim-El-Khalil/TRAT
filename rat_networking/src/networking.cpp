#include "rat/networking.hpp"
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdio>
#include <cstring>

#include "logging.hpp"

namespace rat::networking {

// ------------------------------
// Helpers
// ------------------------------

std::filesystem::path _getFilePathFromUrl(const std::string& arg_Url) {
    std::string filename = arg_Url.substr(arg_Url.find_last_of("/\\") + 1);
    if (filename.empty()) filename = "downloaded_file";
    return std::filesystem::path(filename);
}

// Callbacks (prefixed with _ as requested)
static size_t _cbHeapWrite(void* p_Contents, size_t arg_Size, size_t arg_Nmemb, void* p_User) {
    auto* buffer = static_cast<std::vector<char>*>(p_User);
    const size_t total_size = arg_Size * arg_Nmemb;
    const char* src = static_cast<const char*>(p_Contents);
    buffer->insert(buffer->end(), src, src + total_size);
    return total_size;
}

static size_t _cbFileWrite(void* p_Contents, size_t arg_Size, size_t arg_Nmemb, void* p_User) {
    FILE* fp = static_cast<FILE*>(p_User);
    return std::fwrite(p_Contents, arg_Size, arg_Nmemb, fp);
}

static size_t _cbStackWrite(void* p_Contents, size_t arg_Size, size_t arg_Nmemb, void* p_User) {
    const size_t total_size = arg_Size * arg_Nmemb;
    auto* context = static_cast<BufferContext*>(p_User);

    const size_t space_left = (context->capacity > context->size) ? (context->capacity - context->size) : 0;
    const size_t copy_size  = (total_size < space_left) ? total_size : space_left;

    if (copy_size > 0) {
        std::memcpy(context->buffer + context->size, p_Contents, copy_size);
        context->size += copy_size;
    }

    // Returning fewer bytes than provided tells libcurl we handled only part of it (truncate)
    return copy_size;
}

// ------------------------------
// EasyCurlHandler
// ------------------------------

EasyCurlHandler::EasyCurlHandler()
    : curl(curl_easy_init()), state(CURLE_OK) {}

EasyCurlHandler::~EasyCurlHandler() {
    if (this->curl) {
        curl_easy_cleanup(this->curl);
        this->curl = nullptr;
    }
}

CURLcode EasyCurlHandler::setOption(CURLoption Curl_Option, long value) {
    this->state = curl_easy_setopt(this->curl, Curl_Option, value);
    return this->state;
}

CURLcode EasyCurlHandler::setOption(CURLoption Curl_Option, const char* value) {
    this->state = curl_easy_setopt(this->curl, Curl_Option, value);
    return this->state;
}

CURLcode EasyCurlHandler::setOption(CURLoption Curl_Option, void* value) {
    this->state = curl_easy_setopt(this->curl, Curl_Option, value);
    return this->state;
}

CURLcode EasyCurlHandler::setUrl(const std::string& arg_Url) {
    this->state = curl_easy_setopt(this->curl, CURLOPT_URL, arg_Url.c_str());
    return this->state;
}

CURLcode EasyCurlHandler::setWriteCallBackFunction(WriteCallback Call_Back) {
    this->state = curl_easy_setopt(this->curl, CURLOPT_WRITEFUNCTION, Call_Back);
    return this->state;
}

CURLcode EasyCurlHandler::perform() {
    this->state = curl_easy_perform(this->curl);
    return this->state;
}
void EasyCurlHandler::reset() {
    if(this->curl) {
        curl_easy_reset(this->curl);
    }
}

// ------------------------------
// Client
// ------------------------------

bool Client::download(const std::string& Downloading_Url, const std::filesystem::path& File_Path) {
    if (!this->curl) return false;

    FILE* fp = std::fopen(File_Path.string().c_str(), "wb");
    if (!fp) {
        ERROR_LOG("Failed to open file for writing: %s", File_Path.string().c_str());
        return false;
    }

    setUrl(Downloading_Url);
    setWriteCallBackFunction(&_cbFileWrite);
    setOption(CURLOPT_WRITEDATA, static_cast<void*>(fp));
    setOption(CURLOPT_FOLLOWLOCATION, 1L);
    setOption(CURLOPT_SSL_VERIFYPEER, 1L);
    setOption(CURLOPT_SSL_VERIFYHOST, 2L);
    setOption(CURLOPT_TIMEOUT, 30L);

    this->perform();
    std::fclose(fp);
    this->reset();
    return (this->state == CURLE_OK);
}

bool Client::download(const std::string& Download_Url) {
    // Download into current directory using filename from URL
    auto path = std::filesystem::current_path() / _getFilePathFromUrl(Download_Url);
    return download(Download_Url, path);
}

bool Client::upload(const std::filesystem::path& File_Path, const std::string& Uploading_Url) {
    if (!this->curl || !std::filesystem::exists(File_Path)) return false;

    FILE* fp = std::fopen(File_Path.string().c_str(), "rb");
    if (!fp) {
        ERROR_LOG("Failed to open file for reading: %s", File_Path.string().c_str());
        return false;
    }

    setUrl(Uploading_Url);
    setOption(CURLOPT_UPLOAD, 1L);
    setOption(CURLOPT_READDATA, static_cast<void*>(fp));
    setOption(CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(std::filesystem::file_size(File_Path)));
    setOption(CURLOPT_SSL_VERIFYPEER, 1L);
    setOption(CURLOPT_SSL_VERIFYHOST, 2L);
    setOption(CURLOPT_TIMEOUT, 30L);

    this->perform();
    std::fclose(fp);
    this->reset();
    return (this->state == CURLE_OK);
}

bool Client::uploadMimeFile(const std::filesystem::path& File_Path,
                            const std::string& arg_Url,
                            const std::string& Field_Name,
                            const std::string& Mime_Type) {
    if (!this->curl || !std::filesystem::exists(File_Path)) return false;

    curl_mime* mime = curl_mime_init(this->curl);
    if (!mime) return false;

    curl_mimepart* part = curl_mime_addpart(mime);
    curl_mime_name(part, Field_Name.c_str());
    curl_mime_filedata(part, File_Path.string().c_str());
    if (!Mime_Type.empty()) {
        curl_mime_type(part, Mime_Type.c_str());
    }

    setUrl(arg_Url);
    setOption(CURLOPT_MIMEPOST, static_cast<void*>(mime));
    setOption(CURLOPT_SSL_VERIFYPEER, 1L);
    setOption(CURLOPT_SSL_VERIFYHOST, 2L);
    setOption(CURLOPT_TIMEOUT, 30L);

    this->perform();
    curl_mime_free(mime);

    this->reset();
    return (this->state == CURLE_OK);
}

std::vector<char> Client::sendHttpRequest(const std::string& arg_Url) {
    std::vector<char> buffer;
    if (!this->curl) return buffer;

    this->setUrl(arg_Url);
    this->setWriteCallBackFunction(&_cbHeapWrite);
    this->setOption(CURLOPT_WRITEDATA, static_cast<void*>(&buffer));
    this->setOption(CURLOPT_SSL_VERIFYPEER, 1L);
    this->setOption(CURLOPT_SSL_VERIFYHOST, 2L);
    this->setOption(CURLOPT_FOLLOWLOCATION, 1L);
    this->setOption(CURLOPT_TIMEOUT, 30L);

    this->perform();
    if (this->state != CURLE_OK) buffer.clear();
    
    this->reset();
    return buffer;
}

size_t Client::sendHttpRequest(const std::string& arg_Url, char* p_Buffer, size_t Buffer_Size) {
    if (!this->curl || !p_Buffer || Buffer_Size == 0) return 0;

    BufferContext ctx{p_Buffer, Buffer_Size, 0};

    this->setUrl(arg_Url);
    this->setWriteCallBackFunction(&_cbStackWrite);
    this->setOption(CURLOPT_WRITEDATA, static_cast<void*>(&ctx));
    this->setOption(CURLOPT_SSL_VERIFYPEER, 1L);
    this->setOption(CURLOPT_SSL_VERIFYHOST, 2L);
    this->setOption(CURLOPT_FOLLOWLOCATION, 1L);
    this->setOption(CURLOPT_TIMEOUT, 30L);

    this->perform();
    
    this->reset();
    return (this->state == CURLE_OK) ? ctx.size : 0;
}

static size_t _cbVectorUint8Write(void* p_Contents, size_t arg_Size, size_t arg_Nmemb, void* p_User) {
    auto* buffer = static_cast<std::vector<uint8_t>*>(p_User);
    const size_t total_size = arg_Size * arg_Nmemb;
    const uint8_t* src = static_cast<const uint8_t*>(p_Contents);
    buffer->insert(buffer->end(), src, src + total_size);
    return total_size;
}

bool Client::downloadData(const std::string& arg_Url, std::vector<uint8_t>& Out_Buffer) {
    if (!this->curl) return false;

    Out_Buffer.clear();

    this->setUrl(arg_Url);
    this->setWriteCallBackFunction(&_cbVectorUint8Write);
    this->setOption(CURLOPT_WRITEDATA, static_cast<void*>(&Out_Buffer));
    this->setOption(CURLOPT_SSL_VERIFYPEER, 1L);
    this->setOption(CURLOPT_SSL_VERIFYHOST, 2L);
    this->setOption(CURLOPT_FOLLOWLOCATION, 1L);
    this->setOption(CURLOPT_TIMEOUT, 30L);

    this->perform();
    
    this->reset();
    return (this->state == CURLE_OK);
}

} // namespace rat::networking

