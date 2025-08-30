/*rat_networking/src/networking.cpp*/
#include "rat/networking.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdio>
#include <cstring>

#include "logging.hpp"

namespace rat::networking {

constexpr long CONNECTION_TIMEOUT = 2L;
constexpr long OPERATION_TIMEOUT  = 10L;

void Client::reset() {
    if(this->post_restart_operation_count >= this->operation_restart_bound && !this->is_fresh) {
        this->is_fresh = this->hardResetHandle();         
        this->post_restart_operation_count = 0;
        
        if (this->curl) {
            curl_easy_setopt(this->curl, CURLOPT_MAXCONNECTS, (long)(this->server_endpoints_count));
            curl_easy_setopt(this->curl, CURLOPT_MAXLIFETIME_CONN , 1L);
        }
        return;
    }
    
    this->resetOptions();
    
    if (this->curl) {
        curl_easy_setopt(this->curl, CURLOPT_MAXCONNECTS, (long)(this->server_endpoints_count));
        curl_easy_setopt(this->curl, CURLOPT_MAXLIFETIME_CONN , 1L);
    }
}
bool Client::download(const char* Downloading_Url, const std::filesystem::path& File_Path) {
    if(!Downloading_Url) {
        ERROR_LOG("Downloading_Url is nullptr");
        return false;
    }
    if (!this->curl) {
        ERROR_LOG("CURL handle is not initialized");
        return false;
    }
    FILE* fp = std::fopen(File_Path.string().c_str(), "wb");
    if (!fp) {
        ERROR_LOG("Failed to open file for writing: %s", File_Path.string().c_str());
        return false;
    }
    this->is_fresh = false;
    this->post_restart_operation_count++;
    if (this->setOption(CURLOPT_URL, Downloading_Url) != CURLE_OK) {
        ERROR_LOG("Failed to set URL: %s", curl_easy_strerror(this->state));
        std::fclose(fp);
        this->reset();
        return false;
    }
    if (this->setWriteCallBackFunction(&_cbFileWrite) != CURLE_OK) {
        ERROR_LOG("Failed to set write callback: %s", curl_easy_strerror(this->state));
        std::fclose(fp);
        this->reset();
        return false;
    }
    if (this->setOption(CURLOPT_WRITEDATA, static_cast<void*>(fp)) != CURLE_OK) {
        ERROR_LOG("Failed to set write data: %s", curl_easy_strerror(this->state));
        std::fclose(fp);
        this->reset();
        return false;
    }
    if (this->setOption(CURLOPT_FOLLOWLOCATION, 1L) != CURLE_OK) {
        ERROR_LOG("Failed to set follow location: %s", curl_easy_strerror(this->state));
        std::fclose(fp);
        this->reset();
        return false;
    }
    if (this->setOption(CURLOPT_SSL_VERIFYPEER, 1L) != CURLE_OK) {
        ERROR_LOG("Failed to set SSL peer verification: %s", curl_easy_strerror(this->state));
        std::fclose(fp);
        this->reset();
        return false;
    }
    if (this->setOption(CURLOPT_SSL_VERIFYHOST, 2L) != CURLE_OK) {
        ERROR_LOG("Failed to set SSL host verification: %s", curl_easy_strerror(this->state)); 
        std::fclose(fp);
        this->reset();
        return false;
    }
    if ( (this->setOption(CURLOPT_TIMEOUT, ::rat::networking::OPERATION_TIMEOUT) != CURLE_OK) || 
         (this->setOption(CURLOPT_CONNECTTIMEOUT, ::rat::networking::CONNECTION_TIMEOUT) != CURLE_OK) )  {
        ERROR_LOG("Failed to set timeout: %s", curl_easy_strerror(this->state));
        std::fclose(fp);
        this->reset();
        return false;
    }
    // Only perform if all options were set successfully
    if (this->perform() != CURLE_OK) {
        ERROR_LOG("Download failed: %s", curl_easy_strerror(this->state));
        std::fclose(fp);
        this->reset();
        return false;
    }
    std::fclose(fp);
    this->reset();
    return true;
}

bool Client::upload(const std::filesystem::path& File_Path, const char* Uploading_Url) {
    if(!Uploading_Url) {
        ERROR_LOG("Uploading_Url is nullptr");
        return false;
    }
    if (!this->curl) {
        ERROR_LOG("CURL handle is not initialized");
        return false;
    }
    if (!std::filesystem::exists(File_Path)) {
        ERROR_LOG("File does not exist: %s", File_Path.string().c_str());
        return false;
    }
    FILE* fp = std::fopen(File_Path.string().c_str(), "rb");
    if (!fp) {
        ERROR_LOG("Failed to open file for reading: %s", File_Path.string().c_str());
        return false;
    }
    this->post_restart_operation_count++;
    this->is_fresh = false;

    if (this->setUrl(Uploading_Url) != CURLE_OK) {
        ERROR_LOG("Failed to set URL: %s", curl_easy_strerror(this->state));
        std::fclose(fp);
        this->reset();
        return false;
    }
     if (this->setOption(CURLOPT_UPLOAD, 1L) != CURLE_OK ||
        curl_easy_setopt(this->curl, CURLOPT_WRITEFUNCTION, _cbWriteDiscard) != CURLE_OK)  {
        ERROR_LOG("Failed to set upload option: %s", curl_easy_strerror(this->state));
        std::fclose(fp);
        this->reset();
        return false;
    }
    if (this->setOption(CURLOPT_READDATA, static_cast<void*>(fp)) != CURLE_OK) {
        ERROR_LOG("Failed to set read data: %s", curl_easy_strerror(this->state));
        std::fclose(fp);
        this->reset();
        return false;
    }
    
    curl_off_t file_size = static_cast<curl_off_t>(std::filesystem::file_size(File_Path));
    if (this->setOption(CURLOPT_INFILESIZE_LARGE, file_size) != CURLE_OK) {
        ERROR_LOG("Failed to set file size: %s", curl_easy_strerror(this->state));
        std::fclose(fp);
        this->reset();
        return false;
    }
    if (this->setOption(CURLOPT_SSL_VERIFYPEER, 1L) != CURLE_OK) {
        ERROR_LOG("Failed to set SSL peer verification: %s", curl_easy_strerror(this->state));
        std::fclose(fp);
        this->reset();
        return false;
    }
    if (this->setOption(CURLOPT_SSL_VERIFYHOST, 2L) != CURLE_OK) {
        ERROR_LOG("Failed to set SSL host verification: %s", curl_easy_strerror(this->state));
        std::fclose(fp);
        this->reset();
        return false;
    }
    
    if ( (this->setOption(CURLOPT_TIMEOUT, ::rat::networking::OPERATION_TIMEOUT) != CURLE_OK) || 
         (this->setOption(CURLOPT_CONNECTTIMEOUT, ::rat::networking::CONNECTION_TIMEOUT) != CURLE_OK) ) {
        ERROR_LOG("Failed to set timeout: %s", curl_easy_strerror(this->state));
        std::fclose(fp);
        this->reset();
        return false;
    }

    // Only perform if all options were set successfully
    if (this->perform() != CURLE_OK) {
        ERROR_LOG("Upload failed: %s", curl_easy_strerror(this->state));
        std::fclose(fp);
        this->reset();
        return false;
    }
    std::fclose(fp);
    this->reset();
    return true;
}

bool Client::uploadMimeFile(const MimeContext& Mime_Context) {
    DEBUG_LOG("Client::uploadMimeFile(const MimeContext& Mime_Context) was invoked, now inside its scope");
    if (!this->curl) {
        ERROR_LOG("CURL handle is not initialized");
        return false;
    }
    DEBUG_LOG("Check passed, curl instance exists");
    if (!std::filesystem::exists(Mime_Context.file_path)) {
        ERROR_LOG("File does not exist: %s", Mime_Context.file_path.string().c_str());
        return false;
    }
    this->post_restart_operation_count++;
    this->is_fresh = false;

    DEBUG_LOG("Check passed, file exists");
    curl_mime* mime = curl_mime_init(this->curl);
    if (!mime) {
        ERROR_LOG("Failed to create MIME structure");
        this->reset();
        return false;
    }
    DEBUG_LOG("curl_mime* instance was created");
    // Add file part
    curl_mimepart* file_part = curl_mime_addpart(mime);
    DEBUG_LOG("curl_mimepart* instance was created");
    if (!file_part) {
        ERROR_LOG("Failed to add file MIME part");
        curl_mime_free(mime);
        this->reset();
        return false;
    }
    CURLcode result = curl_mime_name(file_part, Mime_Context.file_field_name.c_str());
    if (result != CURLE_OK) {
        ERROR_LOG("Failed to set file field name: %s", curl_easy_strerror(result));
        curl_mime_free(mime);
        this->reset();
        return false;
    }
    result = curl_mime_filedata(file_part, Mime_Context.file_path.string().c_str());
    if (result != CURLE_OK) {
        ERROR_LOG("Failed to set file data: %s", curl_easy_strerror(result));
        curl_mime_free(mime);
        this->reset();
        return false;
    }
    if (!Mime_Context.mime_type.empty()) {
        result = curl_mime_type(file_part, Mime_Context.mime_type.c_str());
        if (result != CURLE_OK) {
            ERROR_LOG("Failed to set MIME type: %s", curl_easy_strerror(result));
            curl_mime_free(mime);
            this->reset();
            return false;
        }
    }
    // Add all form fields from the map
    for (const auto& [field_name, field_value] : Mime_Context.fields_map) {
        curl_mimepart* field_part = curl_mime_addpart(mime);
        if (!field_part) {
            ERROR_LOG("Failed to add field part for: %s", field_name.c_str());
            curl_mime_free(mime);
            this->reset();
            return false;
        }
        result = curl_mime_name(field_part, field_name.c_str());
        if (result != CURLE_OK) {
            ERROR_LOG("Failed to set field name %s: %s", field_name.c_str(), curl_easy_strerror(result));
            curl_mime_free(mime);
            this->reset();
            return false;
        }
        result = curl_mime_data(field_part, field_value.c_str(), CURL_ZERO_TERMINATED);
        if (result != CURLE_OK) {
            ERROR_LOG("Failed to set field data for %s: %s", field_name.c_str(), curl_easy_strerror(result));
            curl_mime_free(mime);
            this->reset();
            return false;
        }
    }
    DEBUG_LOG("Setting curl options in networking::uploadMimeFile(MimeContext& ctx)");
    if (this->setUrl(Mime_Context.url) != CURLE_OK) {
        ERROR_LOG("Failed to set URL: %s", curl_easy_strerror(this->state));
        curl_mime_free(mime);
        this->reset();
        return false;
    }
    //This is a must be otherwise the response will be given to the standard output
    if (curl_easy_setopt(this->curl, CURLOPT_WRITEFUNCTION, _cbWriteDiscard) != CURLE_OK) {
        ERROR_LOG("Failed to set writefunction: %s", curl_easy_strerror(this->state));
        curl_mime_free(mime);
        this->reset();
        return false;
    }
    if (this->setOption(CURLOPT_MIMEPOST, static_cast<void*>(mime)) != CURLE_OK) {
        ERROR_LOG("Failed to set MIME post option: %s", curl_easy_strerror(this->state));
        curl_mime_free(mime);
        this->reset();
        return false;
    }
    if (this->setOption(CURLOPT_SSL_VERIFYPEER, 1L) != CURLE_OK) {
        ERROR_LOG("Failed to set SSL peer verification: %s", curl_easy_strerror(this->state));
        curl_mime_free(mime);
        this->reset();
        return false;
    }
    if (this->setOption(CURLOPT_SSL_VERIFYHOST, 2L) != CURLE_OK) {
        ERROR_LOG("Failed to set SSL host verification: %s", curl_easy_strerror(this->state));
        curl_mime_free(mime);
        this->reset();
        return false;
    }
    if ( (this->setOption(CURLOPT_TIMEOUT, ::rat::networking::OPERATION_TIMEOUT) != CURLE_OK) || 
         (this->setOption(CURLOPT_CONNECTTIMEOUT, ::rat::networking::CONNECTION_TIMEOUT) != CURLE_OK) ) { 
        ERROR_LOG("Failed to set timeout: %s", curl_easy_strerror(this->state));
        curl_mime_free(mime);
        this->reset();
        return false;
    }

    // Perform the request if all options were set successfully
    if (this->perform() != CURLE_OK) {
        ERROR_LOG("MIME upload failed: %s", curl_easy_strerror(this->state));
        curl_mime_free(mime);
        this->reset();
        return false;
    }
    curl_mime_free(mime);
    /*After a mime operation, since I do not understand its shenanigans exactly, hard reset the handle is the safest option*/
    this->hardResetHandle();
    return true;
}
std::vector<char> Client::sendHttpRequest(const char* arg_Url) {
    if(!arg_Url) {
        ERROR_LOG("arg_Url is nullptr");
        return {};
    }
    std::vector<char> buffer;
    if (!this->curl) {
        ERROR_LOG("CURL handle is not initialized");
        return buffer;
    }
    this->post_restart_operation_count++;
    this->is_fresh = false;
    if (this->setUrl(arg_Url) != CURLE_OK) {
        ERROR_LOG("Failed to set URL: %s", curl_easy_strerror(this->state));
        this->reset();
        return buffer;
    }
    if (this->setWriteCallBackFunction(&_cbHeapWrite) != CURLE_OK) {
        ERROR_LOG("Failed to set write callback: %s", curl_easy_strerror(this->state));
        this->reset();
        return buffer;
    }
    if (this->setOption(CURLOPT_WRITEDATA, static_cast<void*>(&buffer)) != CURLE_OK) {
        ERROR_LOG("Failed to set write data: %s", curl_easy_strerror(this->state));
        this->reset();
        return buffer;
    }
    if (this->setOption(CURLOPT_SSL_VERIFYPEER, 1L) != CURLE_OK) {
        ERROR_LOG("Failed to set SSL peer verification: %s", curl_easy_strerror(this->state));
        this->reset();
        return buffer;
    }
    if (this->setOption(CURLOPT_SSL_VERIFYHOST, 2L) != CURLE_OK) {
        ERROR_LOG("Failed to set SSL host verification: %s", curl_easy_strerror(this->state));
        this->reset();
        return buffer;
    }
    if (this->setOption(CURLOPT_FOLLOWLOCATION, 1L) != CURLE_OK) {
        ERROR_LOG("Failed to set follow location: %s", curl_easy_strerror(this->state));
        this->reset();
        return buffer;
    }
    if ( (this->setOption(CURLOPT_TIMEOUT, ::rat::networking::OPERATION_TIMEOUT) != CURLE_OK) || 
         (this->setOption(CURLOPT_CONNECTTIMEOUT, ::rat::networking::CONNECTION_TIMEOUT) != CURLE_OK) )  {
        ERROR_LOG("Failed to set timeout: %s", curl_easy_strerror(this->state));
        this->reset();
        return buffer;
    }
    if (this->perform() != CURLE_OK) {
        ERROR_LOG("HTTP request failed: %s", curl_easy_strerror(this->state));
        buffer.clear();
        this->reset();
        return buffer;
    }
    this->reset();
    return buffer;
}

size_t Client::sendHttpRequest(const char* arg_Url, char* p_Buffer, size_t Buffer_Size) {
    if(!arg_Url) {
        ERROR_LOG("arg_Url is nullptr");
        return false;
    }
    if (!this->curl) {
        ERROR_LOG("CURL handle is not initialized");
        return 0;
    }
    if (!p_Buffer || Buffer_Size == 0) {
        ERROR_LOG("Invalid buffer parameters");
        return 0;
    }
    BufferContext ctx{p_Buffer, Buffer_Size, 0};
    this->post_restart_operation_count++;
    this->is_fresh = false;
    if (this->setUrl(arg_Url) != CURLE_OK) {
        ERROR_LOG("Failed to set URL: %s", curl_easy_strerror(this->state));
        this->reset();
        return 0;
    }
    if (this->setWriteCallBackFunction(&_cbStackWrite) != CURLE_OK) {
        ERROR_LOG("Failed to set write callback: %s", curl_easy_strerror(this->state));
        this->reset();
        return 0;
    }
    if (this->setOption(CURLOPT_WRITEDATA, static_cast<void*>(&ctx)) != CURLE_OK) {
        ERROR_LOG("Failed to set write data: %s", curl_easy_strerror(this->state));
        this->reset();
        return 0;
    }
    if (this->setOption(CURLOPT_SSL_VERIFYPEER, 1L) != CURLE_OK) {
        ERROR_LOG("Failed to set SSL peer verification: %s", curl_easy_strerror(this->state));
        this->reset();
        return 0;
    }
    if (this->setOption(CURLOPT_SSL_VERIFYHOST, 2L) != CURLE_OK) {
        ERROR_LOG("Failed to set SSL host verification: %s", curl_easy_strerror(this->state));
        this->reset();
        return 0;
    }
    if (this->setOption(CURLOPT_FOLLOWLOCATION, 1L) != CURLE_OK) {
        ERROR_LOG("Failed to set follow location: %s", curl_easy_strerror(this->state));
        this->reset();
        return 0;
    }
    if ( (this->setOption(CURLOPT_TIMEOUT, ::rat::networking::OPERATION_TIMEOUT) != CURLE_OK) || 
         (this->setOption(CURLOPT_CONNECTTIMEOUT, ::rat::networking::CONNECTION_TIMEOUT) != CURLE_OK) ) {
        ERROR_LOG("Failed to set timeout: %s", curl_easy_strerror(this->state));
        this->reset();
        return 0;
    }
    if (this->perform() != CURLE_OK) {
        ERROR_LOG("HTTP request failed: %s", curl_easy_strerror(this->state));
        this->reset();
        return 0;
    }
    this->reset();
    return ctx.size;
}

bool Client::downloadData(const std::string& arg_Url, std::vector<uint8_t>& Out_Buffer) {
    if (!this->curl) {
        ERROR_LOG("CURL handle is not initialized");
        return false;
    }
    Out_Buffer.clear();
    this->post_restart_operation_count++;
    this->is_fresh = false;
    if (this->setUrl(arg_Url) != CURLE_OK) {
        ERROR_LOG("Failed to set URL: %s", curl_easy_strerror(this->state));
        this->reset();
        return false;
    }
    if (this->setWriteCallBackFunction(&_cbVectorUint8Write) != CURLE_OK) {
        ERROR_LOG("Failed to set write callback: %s", curl_easy_strerror(this->state));
        this->reset();
        return false;
    }    
    if (this->setOption(CURLOPT_WRITEDATA, static_cast<void*>(&Out_Buffer)) != CURLE_OK) {
        ERROR_LOG("Failed to set write data: %s", curl_easy_strerror(this->state));
        this->reset();
        return false;
    }
    if (this->setOption(CURLOPT_SSL_VERIFYPEER, 1L) != CURLE_OK) {
        ERROR_LOG("Failed to set SSL peer verification: %s", curl_easy_strerror(this->state));
        this->reset();
        return false;
    }
    if (this->setOption(CURLOPT_SSL_VERIFYHOST, 2L) != CURLE_OK) {
        ERROR_LOG("Failed to set SSL host verification: %s", curl_easy_strerror(this->state));
        this->reset();
        return false;
    }
    if (this->setOption(CURLOPT_FOLLOWLOCATION, 1L) != CURLE_OK) {
        ERROR_LOG("Failed to set follow location: %s", curl_easy_strerror(this->state));
        this->reset();
        return false;
    }
    if ( (this->setOption(CURLOPT_TIMEOUT, ::rat::networking::OPERATION_TIMEOUT) != CURLE_OK) || 
         (this->setOption(CURLOPT_CONNECTTIMEOUT, ::rat::networking::CONNECTION_TIMEOUT) != CURLE_OK) ) {
        ERROR_LOG("Failed to set timeout: %s", curl_easy_strerror(this->state));
        this->reset();
        return false;
    }
    if (this->perform() != CURLE_OK) {
        ERROR_LOG("Download data failed: %s", curl_easy_strerror(this->state));
        Out_Buffer.clear();
        this->reset();
        return false;
    }
    this->reset();
    return true;
}
} // namespace rat::networking
