/* rat_networking/src/networking.cpp */
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

NetworkingResult Client::download(const char* Downloading_Url, const std::filesystem::path& File_Path) {
    NetworkingResult result{ CURLE_OK, 0 };

    if(!Downloading_Url) {
        ERROR_LOG("Downloading_Url is nullptr");
        result.curl_code = CURLE_BAD_FUNCTION_ARGUMENT;
        return result;
    }
    if (!this->curl) {
        ERROR_LOG("CURL handle is not initialized");
        result.curl_code = CURLE_FAILED_INIT;
        return result;
    }
    FILE* fp = std::fopen(File_Path.string().c_str(), "wb");
    if (!fp) {
        ERROR_LOG("Failed to open file for writing: %s", File_Path.string().c_str());
        result.curl_code = CURLE_WRITE_ERROR;
        return result;
    }

    this->is_fresh = false;
    this->post_restart_operation_count++;

    if ((result.curl_code = this->setOption(CURLOPT_URL, Downloading_Url)) != CURLE_OK ||
        (result.curl_code = this->setWriteCallBackFunction(&_cbFileWrite)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_WRITEDATA, static_cast<void*>(fp))) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_FOLLOWLOCATION, 1L)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_SSL_VERIFYPEER, 1L)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_SSL_VERIFYHOST, 2L)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_TIMEOUT, OPERATION_TIMEOUT)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_CONNECTTIMEOUT, CONNECTION_TIMEOUT)) != CURLE_OK) {
        
        ERROR_LOG("Download setup failed: %s", curl_easy_strerror(result.curl_code));
        std::fclose(fp);
        this->reset();
        return result;
    }

    result.curl_code = this->perform();
    if (result.curl_code != CURLE_OK) {
        ERROR_LOG("Download failed: %s", curl_easy_strerror(result.curl_code));
        std::fclose(fp);
        this->reset();
        return result;
    }

    fseek(fp, 0, SEEK_END);
    result.size = std::ftell(fp);
    std::fclose(fp);

    this->reset();
    return result;
}

NetworkingResult Client::upload(const std::filesystem::path& File_Path, const char* Uploading_Url) {
    NetworkingResult result{ CURLE_OK, 0 };

    if(!Uploading_Url) {
        ERROR_LOG("Uploading_Url is nullptr");
        result.curl_code = CURLE_BAD_FUNCTION_ARGUMENT;
        return result;
    }
    if (!this->curl) {
        ERROR_LOG("CURL handle is not initialized");
        result.curl_code = CURLE_FAILED_INIT;
        return result;
    }
    if (!std::filesystem::exists(File_Path)) {
        ERROR_LOG("File does not exist: %s", File_Path.string().c_str());
        result.curl_code = CURLE_READ_ERROR;
        return result;
    }
    FILE* fp = std::fopen(File_Path.string().c_str(), "rb");
    if (!fp) {
        ERROR_LOG("Failed to open file for reading: %s", File_Path.string().c_str());
        result.curl_code = CURLE_READ_ERROR;
        return result;
    }

    this->post_restart_operation_count++;
    this->is_fresh = false;

    if ((result.curl_code = this->setUrl(Uploading_Url)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_UPLOAD, 1L)) != CURLE_OK ||
        (result.curl_code = curl_easy_setopt(this->curl, CURLOPT_WRITEFUNCTION, _cbWriteDiscard)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_READDATA, static_cast<void*>(fp))) != CURLE_OK) {
        
        ERROR_LOG("Failed to configure upload: %s", curl_easy_strerror(result.curl_code));
        std::fclose(fp);
        this->reset();
        return result;
    }

    curl_off_t file_size = static_cast<curl_off_t>(std::filesystem::file_size(File_Path));
    if ((result.curl_code = this->setOption(CURLOPT_INFILESIZE_LARGE, file_size)) != CURLE_OK) {
        ERROR_LOG("Failed to set file size: %s", curl_easy_strerror(result.curl_code));
        std::fclose(fp);
        this->reset();
        return result;
    }

    if ((result.curl_code = this->setOption(CURLOPT_SSL_VERIFYPEER, 1L)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_SSL_VERIFYHOST, 2L)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_TIMEOUT, OPERATION_TIMEOUT)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_CONNECTTIMEOUT, CONNECTION_TIMEOUT)) != CURLE_OK) {
        
        ERROR_LOG("Failed to set security/timeouts: %s", curl_easy_strerror(result.curl_code));
        std::fclose(fp);
        this->reset();
        return result;
    }

    result.curl_code = this->perform();
    if (result.curl_code != CURLE_OK) {
        ERROR_LOG("Upload failed: %s", curl_easy_strerror(result.curl_code));
        std::fclose(fp);
        this->reset();
        return result;
    }

    result.size = std::filesystem::file_size(File_Path);

    std::fclose(fp);
    this->reset();
    return result;
}

NetworkingResult Client::uploadMimeFile(const MimeContext& Mime_Context) {
    NetworkingResult result{ CURLE_OK, 0 };

    if (!this->curl) {
        ERROR_LOG("CURL handle is not initialized");
        result.curl_code = CURLE_FAILED_INIT;
        return result;
    }
    if (!std::filesystem::exists(Mime_Context.file_path)) {
        ERROR_LOG("File does not exist: %s", Mime_Context.file_path.string().c_str());
        result.curl_code = CURLE_READ_ERROR;
        return result;
    }

    this->post_restart_operation_count++;
    this->is_fresh = false;

    curl_mime* mime = curl_mime_init(this->curl);
    if (!mime) {
        ERROR_LOG("Failed to create MIME structure");
        result.curl_code = CURLE_OUT_OF_MEMORY;
        this->reset();
        return result;
    }

    curl_mimepart* file_part = curl_mime_addpart(mime);
    if (!file_part) {
        ERROR_LOG("Failed to add file MIME part");
        curl_mime_free(mime);
        result.curl_code = CURLE_OUT_OF_MEMORY;
        this->reset();
        return result;
    }

    if ((result.curl_code = curl_mime_name(file_part, Mime_Context.file_field_name.c_str())) != CURLE_OK ||
        (result.curl_code = curl_mime_filedata(file_part, Mime_Context.file_path.string().c_str())) != CURLE_OK) {
        ERROR_LOG("Failed to set file MIME part: %s", curl_easy_strerror(result.curl_code));
        curl_mime_free(mime);
        this->reset();
        return result;
    }

    if (!Mime_Context.mime_type.empty()) {
        if ((result.curl_code = curl_mime_type(file_part, Mime_Context.mime_type.c_str())) != CURLE_OK) {
            ERROR_LOG("Failed to set MIME type: %s", curl_easy_strerror(result.curl_code));
            curl_mime_free(mime);
            this->reset();
            return result;
        }
    }

    for (const auto& [field_name, field_value] : Mime_Context.fields_map) {
        curl_mimepart* field_part = curl_mime_addpart(mime);
        if (!field_part) {
            ERROR_LOG("Failed to add field part for: %s", field_name.c_str());
            result.curl_code = CURLE_OUT_OF_MEMORY;
            curl_mime_free(mime);
            this->reset();
            return result;
        }
        if ((result.curl_code = curl_mime_name(field_part, field_name.c_str())) != CURLE_OK ||
            (result.curl_code = curl_mime_data(field_part, field_value.c_str(), CURL_ZERO_TERMINATED)) != CURLE_OK) {
            ERROR_LOG("Failed to set field data for %s: %s", field_name.c_str(), curl_easy_strerror(result.curl_code));
            curl_mime_free(mime);
            this->reset();
            return result;
        }
    }

    if ((result.curl_code = this->setUrl(Mime_Context.url)) != CURLE_OK ||
        (result.curl_code = curl_easy_setopt(this->curl, CURLOPT_WRITEFUNCTION, _cbWriteDiscard)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_MIMEPOST, static_cast<void*>(mime))) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_SSL_VERIFYPEER, 1L)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_SSL_VERIFYHOST, 2L)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_TIMEOUT, OPERATION_TIMEOUT)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_CONNECTTIMEOUT, CONNECTION_TIMEOUT)) != CURLE_OK) {
        
        ERROR_LOG("Failed to configure MIME upload: %s", curl_easy_strerror(result.curl_code));
        curl_mime_free(mime);
        this->reset();
        return result;
    }

    result.curl_code = this->perform();
    if (result.curl_code != CURLE_OK) {
        ERROR_LOG("MIME upload failed: %s", curl_easy_strerror(result.curl_code));
        curl_mime_free(mime);
        this->reset();
        return result;
    }

    result.size = std::filesystem::file_size(Mime_Context.file_path);

    curl_mime_free(mime);
    this->hardResetHandle();
    return result;
}

NetworkingResult Client::sendHttpRequest(const char* arg_Url, std::vector<char>& arg_Buffer) {
    NetworkingResult result{ CURLE_OK, 0 };
    arg_Buffer.clear();

    if(!arg_Url) {
        ERROR_LOG("arg_Url is nullptr");
        result.curl_code = CURLE_BAD_FUNCTION_ARGUMENT;
        return result;
    }
    if (!this->curl) {
        ERROR_LOG("CURL handle is not initialized");
        result.curl_code = CURLE_FAILED_INIT;
        return result;
    }

    this->post_restart_operation_count++;
    this->is_fresh = false;

    if ((result.curl_code = this->setUrl(arg_Url)) != CURLE_OK ||
        (result.curl_code = this->setWriteCallBackFunction(&_cbHeapWrite)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_WRITEDATA, static_cast<void*>(&arg_Buffer))) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_SSL_VERIFYPEER, 1L)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_SSL_VERIFYHOST, 2L)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_FOLLOWLOCATION, 1L)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_TIMEOUT, OPERATION_TIMEOUT)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_CONNECTTIMEOUT, CONNECTION_TIMEOUT)) != CURLE_OK) {
        
        ERROR_LOG("HTTP setup failed: %s", curl_easy_strerror(result.curl_code));
        this->reset();
        return result;
    }

    result.curl_code = this->perform();
    if (result.curl_code != CURLE_OK) {
        ERROR_LOG("HTTP request failed: %s", curl_easy_strerror(result.curl_code));
        arg_Buffer.clear();
        this->reset();
        return result;
    }

    result.size = arg_Buffer.size();
    this->reset();
    return result;
}

NetworkingResult Client::sendHttpRequest(const char* arg_Url, char* p_Buffer, size_t Buffer_Size) {
    NetworkingResult result{ CURLE_OK, 0 };

    if(!arg_Url || !p_Buffer || Buffer_Size == 0) {
        ERROR_LOG("Bad arguments to sendHttpRequest");
        result.curl_code = CURLE_BAD_FUNCTION_ARGUMENT;
        return result;
    }
    if (!this->curl) {
        ERROR_LOG("CURL handle is not initialized");
        result.curl_code = CURLE_FAILED_INIT;
        return result;
    }

    BufferContext ctx{p_Buffer, Buffer_Size, 0};
    this->post_restart_operation_count++;
    this->is_fresh = false;

    if ((result.curl_code = this->setUrl(arg_Url)) != CURLE_OK ||
        (result.curl_code = this->setWriteCallBackFunction(&_cbStackWrite)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_WRITEDATA, static_cast<void*>(&ctx))) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_SSL_VERIFYPEER, 1L)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_SSL_VERIFYHOST, 2L)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_FOLLOWLOCATION, 1L)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_TIMEOUT, OPERATION_TIMEOUT)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_CONNECTTIMEOUT, CONNECTION_TIMEOUT)) != CURLE_OK) {
        
        ERROR_LOG("Failed to configure HTTP request: %s", curl_easy_strerror(result.curl_code));
        this->reset();
        return result;
    }

    result.curl_code = this->perform();
    if (result.curl_code != CURLE_OK) {
        ERROR_LOG("HTTP request failed: %s", curl_easy_strerror(result.curl_code));
        this->reset();
        return result;
    }

    result.size = ctx.size;
    this->reset();
    return result;
}

NetworkingResult Client::downloadData(const std::string& arg_Url, std::vector<uint8_t>& Out_Buffer) {
    NetworkingResult result{ CURLE_OK, 0 };
    Out_Buffer.clear();

    if (!this->curl) {
        ERROR_LOG("CURL handle is not initialized");
        result.curl_code = CURLE_FAILED_INIT;
        return result;
    }

    this->post_restart_operation_count++;
    this->is_fresh = false;

    if ((result.curl_code = this->setUrl(arg_Url)) != CURLE_OK ||
        (result.curl_code = this->setWriteCallBackFunction(&_cbVectorUint8Write)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_WRITEDATA, static_cast<void*>(&Out_Buffer))) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_SSL_VERIFYPEER, 1L)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_SSL_VERIFYHOST, 2L)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_FOLLOWLOCATION, 1L)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_TIMEOUT, OPERATION_TIMEOUT)) != CURLE_OK ||
        (result.curl_code = this->setOption(CURLOPT_CONNECTTIMEOUT, CONNECTION_TIMEOUT)) != CURLE_OK) {
        
        ERROR_LOG("DownloadData setup failed: %s", curl_easy_strerror(result.curl_code));
        this->reset();
        return result;
    }

    result.curl_code = this->perform();
    if (result.curl_code != CURLE_OK) {
        ERROR_LOG("Download data failed: %s", curl_easy_strerror(result.curl_code));
        Out_Buffer.clear();
        this->reset();
        return result;
    }

    result.size = Out_Buffer.size();
    this->reset();
    return result;
}

} // namespace rat::networking
