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
			uint32_t HandlerId;
			uint64_t BuildNumber;
#if false
			uint32_t ChunkCount;
#else
			uint64_t MetadataHeapSize;
#endif
			uint8_t _Padding[32];

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

		struct MetadataHeader
		{
			FileFormat::RelativePtr<void*> MetadataChunk;

			uint32_t NumChunks;
			FileFormat::RelativePtr<ResourceFileFormat::Chunk> Chunks;

			uint32_t NumStrings;
			FileFormat::RelativePtr<FileFormat::StringEntry> StringEntries;

			uint32_t NumDependencies;
			FileFormat::RelativePtr<uint32_t> Dependencies;
		};

	}
}