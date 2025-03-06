#pragma once

#include <PhxCore/VFS.h>
#include <PhxResource/ResourceFileFormat.h>
namespace phx
{
	struct CompiledResource
	{
		std::string Name;
		std::string Ext;
		std::vector<std::pair<uint32_t, std::unique_ptr<IBlob>>> Chunks;
	};
}