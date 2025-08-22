#include <cstddef>
#include <cstdint>
#include <cstring>

namespace rat::encryption {

uint8_t xorByte(uint8_t arg_Byte, const char* arg_Key, size_t arg_Index);
void xorData(uint8_t* p_Bytes, size_t Number_Bytes, const char* arg_Key);
}//rat
