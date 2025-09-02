/*rat_networking/include/rat/networking/helper.hpp */
#pragma once
#include <cstdlib>
#include <filesystem>
#include <string>

#include "rat/networking/types.hpp"

namespace rat::networking {
/*Callbacks for curl */
std::filesystem::path _getFilePathFromUrl(const std::string& arg_Url);
size_t _cbWriteDiscard(void* p_Contents, size_t arg_Size, size_t arg_Nmemb, void* p_User);
size_t _cbHeapWrite(void* p_Contents, size_t arg_Size, size_t arg_Nmemb, void* p_User);
size_t _cbFileWrite(void* p_Contents, size_t arg_Size, size_t arg_Nmemb, void* p_User);
size_t _cbStackWrite(void* p_Contents, size_t arg_Size, size_t arg_Nmemb, void* p_User);
size_t _cbVectorUint8Write(void* p_Contents, size_t arg_Size, size_t arg_Nmemb, void* p_User);
size_t _cbVectorCharWrite(void *p_Contents, size_t arg_Size, size_t arg_Nmemb, void *p_User);
}//namespace rat::networking
