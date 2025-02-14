#pragma once

#include <PhxCore/Span.h>

#include <memory>
#include <string>
#include <map>

namespace phx
{
	class IBlob;
	class PakFileBuilder
	{
	public:
		PakFileBuilder() = default;
		~PakFileBuilder() = default;

	public:
		PakFileBuilder& AddFiles(Span<std::pair<std::string, IBlob*>> entry)
		{
			for (auto& pair : entry)
				m_entries.emplace(pair);

			return *this;
		}

		std::unique_ptr<IBlob> Build();

	private:
		std::map<std::string, IBlob*> m_entries;
	};
}

