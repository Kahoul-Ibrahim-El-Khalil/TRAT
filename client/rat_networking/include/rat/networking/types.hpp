/*rat_networking/include/rat/networking/types.hpp*/

#pragma once

#include <cstdlib>
#include <curl/curl.h>
#include <filesystem>
#include <map>
#include <string>

namespace rat::networking {
struct BufferContext {
	char *buffer;
	size_t capacity;
	size_t size;
};

struct MimeContext {
	std::filesystem::path file_path;
	std::string url;
	std::map<std::string, std::string> fields_map;
	std::string file_field_name = "document"; // default field name for file
	std::string mime_type = "";				  // optional MIME type
};

struct NetworkingResult {
	CURLcode curl_code;
	size_t size;
};

} // namespace rat::networking
