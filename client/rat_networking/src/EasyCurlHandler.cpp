#include "rat/networking.hpp"
#include "rat/networking/debug.hpp"

namespace rat::networking {

EasyCurlHandler::EasyCurlHandler() : curl(curl_easy_init()), state(CURLE_OK) {
}

EasyCurlHandler::~EasyCurlHandler() {
		if(this->curl) {
			curl_easy_cleanup(this->curl);
			this->curl = nullptr;
		}
}

CURLcode EasyCurlHandler::setOption(CURLoption Curl_Option, long value) {
	this->state = curl_easy_setopt(this->curl, Curl_Option, value);
	return this->state;
}

CURLcode EasyCurlHandler::setOption(CURLoption Curl_Option, const char *value) {
	this->state = curl_easy_setopt(this->curl, Curl_Option, value);
	return this->state;
}

CURLcode EasyCurlHandler::setOption(CURLoption Curl_Option, void *value) {
	this->state = curl_easy_setopt(this->curl, Curl_Option, value);
	return this->state;
}

CURLcode EasyCurlHandler::setUrl(const std::string &arg_Url) {
	this->state = curl_easy_setopt(this->curl, CURLOPT_URL, arg_Url.c_str());
	return this->state;
}

CURLcode EasyCurlHandler::setUrl(const char *arg_Url) {
	this->state = curl_easy_setopt(this->curl, CURLOPT_URL, arg_Url);
	return this->state;
}

CURLcode EasyCurlHandler::setWriteCallBackFunction(WriteCallback Call_Back) {
	this->state =
	    curl_easy_setopt(this->curl, CURLOPT_WRITEFUNCTION, Call_Back);
	return this->state;
}

CURLcode EasyCurlHandler::perform() {
	this->state = curl_easy_perform(this->curl);
		if(this->state != CURLE_OK) {
			NETWORKING_ERROR_LOG(
			    "Failed at performing inside EasyCurlHandler::perform()");
		}
	return this->state;
}

void EasyCurlHandler::resetOptions() {
		if(this->curl) {
			curl_easy_reset(this->curl);
		}
}

} // namespace rat::networking

#undef NETWORKING_DEBUG_LOG
#undef NETWORKING_ERROR_LOG
#ifdef DEBUG_RAT_NETWORKING
#undef DEBUG_RAT_NETWORKING
#endif
