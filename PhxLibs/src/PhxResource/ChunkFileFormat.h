#pragma once

#include "FileFormatUtils.h"

namespace phx
{
	namespace ChunkFileFormat
	{
		/*
				+----------------------+  <--- Start of File
				|   File Header        |  (Fixed Size)
				|----------------------|
				|  Chunk Table (N)     |  (List of ChunkHeaders)
				|----------------------|
				|  Chunk Region (1)    |  (Raw Chunk Data)
				|----------------------|
				|  Chunk Region (1-n)  |
				|----------------------|
				|  Chunk Region (n)    |
				+----------------------+
		*/
		struct Header
		{
			uint32_t Magic;
			uint32_t Version;
			uint64_t BuildNumber;
			uint32_t ChunkCount;
			uint32_t _Pading;
		};

		struct ChunkHeader
		{
			uint32_t ChunkID;
			FileFormat::CompressionType Compression;
			uint8_t _Padding;
			uint32_t Offset; // (on disk this is compressed, in memory it is uncompressed)
			uint32_t CompressedSize;
			uint32_t UncompressedSize;
		};

	}
}