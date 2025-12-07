#pragma once

#include <PhxCore/VFS.h>

namespace phx
{
	struct CompiledResource
	{
		std::string Name;
		std::string Ext;

		// Keep metadata chunk separate as they are stored differently in pak files.
		std::unique_ptr<IBlob> MetadataChunk;
		std::vector<std::unique_ptr<IBlob>> Chunks;
	};
}