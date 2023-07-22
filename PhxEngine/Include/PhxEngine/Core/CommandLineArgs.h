#pragma once

#include <unordered_map>
#include <string>

namespace PhxEngine::Core
{
	class CommandLineArgs
	{
	public:
		CommandLineArgs(int argc, const char** argv);

		bool HasArg(std::string const& key);
		bool GetInteger(std::string const& key, int32_t& value);
		bool GetString(std::string const& key, std::string& value);

	private:
		template<typename FunctionType>
		bool Lookup(std::string const& key, const FunctionType& pfnFunction)
		{
			const auto found = this->m_arguments.find(key);
			if (found != this->m_arguments.end())
			{
				pfnFunction(found->second);
				return true;
			}
			return false;
		}

		template<typename GetStringFunc>
		void GatherArgs(size_t numArgs, const GetStringFunc& pfnGet)
		{
			// the first arg is always the exe name and we are looing for key/value pairs
			if (numArgs > 1)
			{
				size_t i = 0;

				while (i < numArgs)
				{
					const char* key = pfnGet(i);
					i++;
					if (i < numArgs && key[0] == '-')
					{
						const char* strippedKey = key + 1;
						this->m_arguments[strippedKey] = pfnGet(i);
						i++;
					}
				}
			}
		}

	private:
		std::unordered_map<std::string, std::string> m_arguments;
	};
}