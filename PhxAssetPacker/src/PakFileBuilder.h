#pragma once

#include <PhxCore/VFS.h>
#include <PhxCore/Span.h>

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
				m_entries.emplace(pair);
				m_entiresSize += pair.second->Size();
				m_stringDataSize += pair.first.size() + 1; // add one for null terminated string
			}

			return *this;
		}

		std::unique_ptr<IBlob> Build();

	private:
		std::map<std::string, IBlob*> m_entries;
		size_t m_entiresSize = 0ull;
		size_t m_stringDataSize = 0ull;
	};
}

