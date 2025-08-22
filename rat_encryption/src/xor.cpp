#include "rat/encryption/xor.hpp"
#include <cstdint>


namespace rat::encryption {

uint8_t xorByte(uint8_t arg_Byte, const char* arg_Key, size_t arg_Index) {
    if (!arg_Key) return arg_Byte;  // no key? return unchanged
    size_t key_len = std::strlen(arg_Key);
    if (key_len == 0) return arg_Byte; // empty key safety
    return arg_Byte ^ static_cast<uint8_t>(arg_Key[arg_Index % key_len]);
}


void xorData(uint8_t* p_Data, size_t Data_Size, const char* arg_Key) {
    if (!p_Data || !arg_Key) return;

    size_t key_len = std::strlen(arg_Key);
    if (key_len == 0) return;

    for (size_t i = 0; i < Data_Size; ++i) {
        p_Data[i] ^= static_cast<uint8_t>(arg_Key[i % key_len]);
    }
}


}
