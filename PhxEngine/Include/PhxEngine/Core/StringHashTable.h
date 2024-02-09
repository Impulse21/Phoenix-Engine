#pragma once

#include <string>
#include <PhxEngine/Core/StringHash.h>

constexpr const char* StringHashTableFilePath = "C:\\Users\\dipao\\source\\repos\\Impulse21\\Phoenix-Engine\\Assets\\hash_table.json";

namespace PhxEngine::Core
{
	namespace StringHashTable
	{
		void Import(std::string const& file);
		void Export(std::string const& file);

		std::string const& GetName(StringHash hash);
		void RegisterHash(StringHash, std::string const& name);
	}
}

