/*rat_networking/include/rat/networking/types.hpp*/

#pragma once

#include <string>
#include <map>
#include <filesystem>
#include <cstdlib>

namespace rat::networking {
struct BufferContext {
    char*  buffer;     
    size_t capacity;   
    size_t size;       
};

struct MimeContext {
    std::filesystem::path file_path;
    std::string url;
    std::map<std::string, std::string> fields_map;
    std::string file_field_name = "document"; // default field name for file
    std::string mime_type = "";               // optional MIME type
};
}