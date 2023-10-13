#pragma once

#include <unordered_map>
#include <string>

namespace PhxEngine::Core
{
	namespace CommandLineArgs
	{
		void Initialize();

		bool HasArg(std::string const& key);
		bool GetInteger(std::string const& key, int32_t& value);
		bool GetString(std::string const& key, std::string& value);
	}

}