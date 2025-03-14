#pragma once

#include "FileFormatUtils.h"

namespace phx
{
	namespace ResourceFileFormat
	{
		constexpr uint32_t Version = 1;
		constexpr uint32_t MagicNumber = FileFormat::MakeMagicNum('P', 'X', 'R', 'S');

		/*
				+-----------------------+  <--- Start of File
				|   Header        		|  (Fixed Size)
				|-----------------------|
				|  	Chunks    			|  (List of Chunks)
				|-----------------------|
				|   Chunk Data Heap     |  (Raw Chunk Data)
				+-----------------------+
		*/
		struct Header
		{
			uint32_t Magic;
			uint32_t Version;
			uint64_t BuildNumber;
			uint32_t HandlerId;
			uint32_t ChunkCount;
			uint8_t _Padding[40];

		};
        CompileTimeAssertSize(Header, 64);

		// Note Metadata Chunk is moved when Packed
		struct Chunk
		{
			FileFormat::CompressionType		Compression;
			FileFormat::Ptr<void, uint64_t>	Offset;
			uint64_t						CompressedSize;
			uint64_t						UncompressedSize;
		};

	}
}