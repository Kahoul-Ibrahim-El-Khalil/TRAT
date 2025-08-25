// rat_compression/include/rat/compression.hpp

#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>

#include <zlib.h>

#define DEFAULT_COMPRESSION_LEVEL static_cast<uint8_t>(Z_DEFAULT_COMPRESSION)

namespace rat::compression {
/*
uncompressed_size = 0 → original size unknown
compressed_size   = 0 → compressed size unknown
is_compressed     = true → data currently holds compressed bytes
is_compressed     = false → data currently holds uncompressed bytes;
*/

/*This type is not supposed to own or manage the memory block it points to, simply because it is by design type agnostic, it could be a vector, array, queque, stack array .., resizing is the 
duty of the caller*/
struct CompressionContext {
    uint8_t* data;       // pointer to the data buffer
    size_t uncompressed_size;
    size_t compressed_size;
    bool is_compressed;

    uint8_t compression_level; // 0–9, corresponds to zlib levels

    CompressionContext(uint8_t* p_Data, size_t Uncompressed_Size, size_t Compressed_Size, bool Is_Compressed, uint8_t level = DEFAULT_COMPRESSION_LEVEL)
        : data(p_Data),
          uncompressed_size(Uncompressed_Size),
          compressed_size(Compressed_Size),
          is_compressed(Is_Compressed),
          compression_level(level) {}
};

//There is no deconstructor since it is not supposed to actually destroy the data, it only refrence it;

// Compresses data in CompressionContext in-place (requires allocated buffer for compressed data)
bool zlibCompress(CompressionContext& Compression_Context);

// Decompresses data in CompressionContext in-place (requires allocated buffer for decompressed data)
bool zlibDecompress(CompressionContext& Compression_Context);

// functions for convenience sake.
/*The size info will be lost, it must stored beforehand */
bool zlibCompressVector(std::vector<uint8_t>& arg_Vector, int Compression_Level);

bool zlibDecompressVector(std::vector<uint8_t>& arg_Vector, size_t Uncompressed_Size);


} // namespace rat::compression
