// rat_compression/src/compression.cpp

#include "rat/compression.hpp"

#include <cstdint>
#include <vector>
#include <zlib.h>

namespace rat::compression {

using CompressionContext = ::rat::compression::CompressionContext;

bool zlibCompress(CompressionContext &Compression_Context) {
	if(Compression_Context.is_compressed ||
	   Compression_Context.uncompressed_size == 0 || !Compression_Context.data)
		return false;

	uLongf dest_len = static_cast<uLongf>(Compression_Context.compressed_size);
	int ret = compress2(
	    Compression_Context.data, // destination buffer (pre-allocated)
	    &dest_len,                // will contain compressed size
	    Compression_Context.data, // source buffer
	    static_cast<uLong>(Compression_Context.uncompressed_size),
	    static_cast<int>(Compression_Context.compression_level));

	if(ret != Z_OK)
		return false;

	Compression_Context.compressed_size = dest_len;
	Compression_Context.is_compressed = true;
	return true;
}

bool zlibDecompress(CompressionContext &Compression_Context) {
	if(!Compression_Context.is_compressed ||
	   Compression_Context.compressed_size == 0 || !Compression_Context.data)
		return false;

	uLongf dest_len =
	    static_cast<uLongf>(Compression_Context.uncompressed_size);
	int ret = uncompress(
	    Compression_Context.data, // destination buffer (pre-allocated)
	    &dest_len,                // size of destination buffer
	    Compression_Context.data, // source buffer
	    static_cast<uLong>(Compression_Context.compressed_size));

	if(ret != Z_OK)
		return false;

	Compression_Context.is_compressed = false;
	return true;
}

bool zlibCompressVector(std::vector<uint8_t> &arg_Vector,
                        int Compression_Level) {
	if(Compression_Level < 0 || Compression_Level > 9)
		return false;

	CompressionContext compression_context(arg_Vector.data(), arg_Vector.size(),
	                                       0, false, Compression_Level);

		if(zlibCompress(compression_context)) {
			arg_Vector.resize(compression_context.compressed_size);
			return true;
		}
		else {
			return false;
		}
}

bool zlibDecompressVector(std::vector<uint8_t> &arg_Vector,
                          size_t Uncompressed_Size) {
	const size_t compressed_size = arg_Vector.size();
	arg_Vector.resize(Uncompressed_Size);
	CompressionContext compression_context(arg_Vector.data(), Uncompressed_Size,
	                                       compressed_size, true);

		if(zlibDecompress(compression_context)) {
			return true;
		}
		else {
			arg_Vector.resize(compressed_size);
			return false;
		}
}

// Compresses data in CompressionContext using Zstandard
bool zstdCompress(CompressionContext &Compression_Context,
                  int compressionLevel) {
	if(Compression_Context.is_compressed ||
	   Compression_Context.uncompressed_size == 0 || !Compression_Context.data)
		return false;

	// Get maximum compressed size
	size_t maxCompressedSize =
	    ZSTD_compressBound(Compression_Context.uncompressed_size);
	if(Compression_Context.compressed_size < maxCompressedSize)
		return false; // buffer too small

	size_t compressedSize = ZSTD_compress(
	    Compression_Context.data, Compression_Context.compressed_size,
	    Compression_Context.data, Compression_Context.uncompressed_size,
	    compressionLevel);

	if(ZSTD_isError(compressedSize))
		return false;

	Compression_Context.compressed_size = compressedSize;
	Compression_Context.is_compressed = true;
	return true;
}

// Decompresses data in CompressionContext using Zstandard
bool zstdDecompress(CompressionContext &Compression_Context) {
	if(!Compression_Context.is_compressed ||
	   Compression_Context.compressed_size == 0 || !Compression_Context.data)
		return false;

	size_t decompressedSize = ZSTD_decompress(
	    Compression_Context.data, Compression_Context.uncompressed_size,
	    Compression_Context.data, Compression_Context.compressed_size);

	if(ZSTD_isError(decompressedSize))
		return false;

	Compression_Context.is_compressed = false;
	return true;
}

// Convenience function: compress std::vector<uint8_t> using Zstandard
bool zstdCompressVector(std::vector<uint8_t> &arg_Vector,
                        int compressionLevel) {
	size_t originalSize = arg_Vector.size();
	size_t maxCompressedSize = ZSTD_compressBound(originalSize);
	std::vector<uint8_t> buffer(maxCompressedSize);

	CompressionContext ctx(buffer.data(), originalSize, maxCompressedSize,
	                       false);

	if(!zstdCompress(ctx, compressionLevel))
		return false;

	arg_Vector.assign(buffer.begin(), buffer.begin() + ctx.compressed_size);
	return true;
}

// Convenience function: decompress std::vector<uint8_t> using Zstandard
bool zstdDecompressVector(std::vector<uint8_t> &arg_Vector,
                          size_t Uncompressed_Size) {
	std::vector<uint8_t> buffer(Uncompressed_Size);
	CompressionContext ctx(buffer.data(), Uncompressed_Size, arg_Vector.size(),
	                       true);

	if(!zstdDecompress(ctx))
		return false;

	arg_Vector.swap(buffer);
	return true;
}

} // namespace rat::compression
