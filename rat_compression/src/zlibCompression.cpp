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

} // namespace rat::compression
