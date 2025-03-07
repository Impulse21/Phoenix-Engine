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
				std::string filename = std::format("{}{}", resource.Name, resource.Ext);
				Entry e = {
					.Filename = filename,
					.Resource = &resource
				};

				m_entries.emplace(StringHash(filename).ToHash(), e);

				// -- Some upfront size calculations ---
				m_stringHeapSize += filename.size() + 1; // add one for null terminated string

				m_metadataChunksSize += resource.MetadataChunk->Size();
				for (size_t i = 0; i < resource.Chunks.size(); i++)
				{
						m_numChunks += 1;
						m_chunkHeapSize += resource.Chunks[i]->Size();
				}
			}

			return *this;
		}

		std::unique_ptr<IBlob> Build();
		
	private:
		struct Entry
		{
			std::string Filename;
			const CompiledResource* Resource;
		};

		std::map<uint32_t, Entry> m_entries;
		size_t m_numChunks = 0ull;
		size_t m_metadataChunksSize = 0ull;
		size_t m_chunkHeapSize = 0ull;
		size_t m_stringHeapSize = 0ull;
	};
}

