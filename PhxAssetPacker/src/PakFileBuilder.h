#pragma once

#include <PhxCore/VFS.h>
#include <PhxCore/Span.h>
#include <PhxCore/StringHash.h>

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
		PakFileBuilder& AddFiles(Span<std::pair<std::string, IBlob*>> entry)
		{
			for (auto& pair : entry)
			{
				phx::StringHash hash(pair.first);
				m_entries.emplace(std::make_pair(hash.ToHash(), pair));
				m_entiresSize += pair.second->Size();
				m_stringHeapSize += pair.first.size() + 1; // add one for null terminated string
			}

			return *this;
		}

		std::unique_ptr<IBlob> Build();

	private:
		std::map<uint32_t, std::pair<std::string, IBlob*>> m_entries;
		size_t m_entiresSize = 0ull;
		size_t m_stringHeapSize = 0ull;
	};
}

