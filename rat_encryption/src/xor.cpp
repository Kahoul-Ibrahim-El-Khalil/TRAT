#include "rat/encryption/xor.hpp"
#include <cstdint>


namespace rat::encryption {


void xorData(uint8_t* p_Data, size_t Data_Size, const char* arg_Key) {
    if (!p_Data || !arg_Key) return;

    size_t key_len = std::strlen(arg_Key);
    if (key_len == 0) return;

    for (size_t i = 0; i < Data_Size; ++i) {
        p_Data[i] ^= static_cast<uint8_t>(arg_Key[i % key_len]);
    }
}


}
