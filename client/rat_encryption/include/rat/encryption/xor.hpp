#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace rat::encryption {

// XOR a single char at compile time
constexpr char xorChar(char c, char key) {
	return static_cast<char>(c ^ key);
}

// Compile-time XOR encrypt
template <size_t N, size_t K>
constexpr auto xorEncrypt(const char (&str)[N], const char (&key)[K]) {
	std::array<char, N> encrypted{};
		for(size_t i = 0; i < N; i++) {
			encrypted[i] = str[i] ^ key[i % (K - 1)];
		}
	return encrypted;
}

void xorData(uint8_t *p_Bytes, size_t Number_Bytes, const char *arg_Key);

} // namespace rat::encryption
