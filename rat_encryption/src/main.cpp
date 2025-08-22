#include "rat/encryption/xor.hpp"
#include <cstddef>
#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <string>

// Standard Base64 table
static const char b64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode(const uint8_t* data, size_t len) {
    std::string result;
    int val=0, valb=-6;
    for (size_t i = 0; i < len; ++i) {
        val = (val<<8) + data[i];
        valb += 8;
        while (valb >= 0) {
            result.push_back(b64_table[(val>>valb)&0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) result.push_back(b64_table[((val<<8)>>(valb+8))&0x3F]);
    while (result.size()%4) result.push_back('=');
    return result;
}

std::vector<uint8_t> base64_decode(const std::string &s) {
    std::vector<int> T(256,-1);
    for(int i=0;i<64;i++) T[(unsigned char)b64_table[i]] = i;

    std::vector<uint8_t> result;
    int val=0, valb=-8;
    for(unsigned char c : s) {
        if(T[c] == -1) break;
        val = (val<<6) + T[c];
        valb += 6;
        if(valb >= 0) {
            result.push_back((val>>valb)&0xFF);
            valb -= 8;
        }
    }
    return result;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <key> <text>" << std::endl;
        return 1;
    }

    const char* key = argv[1];
    std::string input = argv[2];

    // Convert input to bytes
    std::vector<uint8_t> bytes(input.begin(), input.end());

    // XOR the bytes
    rat::encryption::xorData(bytes.data(), bytes.size(), key);

    // Encode to Base64 for printable representation
    std::string encoded = base64_encode(bytes.data(), bytes.size());
    std::cout << "XOR + Base64 encoded: " << encoded << std::endl;

    // Decode back from Base64
    std::vector<uint8_t> decoded_bytes = base64_decode(encoded);

    // XOR again to recover original text
    rat::encryption::xorData(decoded_bytes.data(), decoded_bytes.size(), key);

    std::string recovered(decoded_bytes.begin(), decoded_bytes.end());
    std::cout << "Recovered text: " << recovered << std::endl;

    return 0;
}
