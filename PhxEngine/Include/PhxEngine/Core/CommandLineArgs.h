#pragma once

#include <string>

namespace PhxEngine
{
	namespace CommandLineArgs
	{
		void Initialize();

		bool HasArg(std::string const& key);
		bool GetInteger(std::string const& key, int32_t& value);
		bool GetString(std::string const& key, std::string& value);
	}

}