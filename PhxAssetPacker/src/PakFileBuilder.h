#pragma once

#include <PhxCore/VFS.h>
#include <PhxCore/Span.h>
#include <PhxCore/StringHash.h>

#include <PhxResource/ResourceFileFormat.h>
#include "CompiledResource.h"
#include <memory>
#include <string>
#include <map>

namespace phx
{
	class PakFileBuilder
	{
	public:
		PakFileBuilder() = default;
		~PakFileBuilder() = default;

	public:
		PakFileBuilder& AddCompiledResources(Span<CompiledResource> resources)
		{
			for (auto& resource : resources)
			{
				std::string filename = std::format("{}.{}", resource.Name, resource.Ext);
				m_entries.emplace(filename, &resource);

				// -- Some upfront size calculations ---
				m_stringHeapSize += filename.size() + 1; // add one for null terminated string

				m_metadataChunksSize += resource.MetadataChunk->Size();
				for (auto& chunk : resource.Chunks)
				{
						m_numChunks += 1;
						m_chunkHeapSize += chunk->Size();
				}
			}

			return *this;
		}

		std::unique_ptr<IBlob> Build();
		
	private:
		std::map<std::string, const CompiledResource*> m_entries;
		size_t m_numChunks = 0ull;
		size_t m_metadataChunksSize = 0ull;
		size_t m_chunkHeapSize = 0ull;
		size_t m_stringHeapSize = 0ull;
	};
}

