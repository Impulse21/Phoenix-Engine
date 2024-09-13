#include "pch.h"
#include "phxCommandLineArgs.h"

#include <unordered_map>

namespace
{
	std::unordered_map<std::wstring, std::wstring> m_argumentMap;
}

namespace CommandLineArgs
{

	template<typename GetStringFunc>
	void GatherArgs(size_t numArgs, const GetStringFunc& pfnGet)
	{
		// the first arg is always the exe name and we are looing for key/value pairs
		if (numArgs > 1)
		{
			size_t i = 0;

			while (i < numArgs)
			{
				const wchar_t* key = pfnGet(i);
				i++;
				if (i < numArgs && key[0] == '-')
				{
					const wchar_t* strippedKey = key + 1;
					m_argumentMap[strippedKey] = pfnGet(i);
					i++;
				}
			}
		}
	}

	void Initialize(int argc, wchar_t** argv)
	{
		GatherArgs(size_t(argc), [argv](size_t i) -> const wchar_t* { return argv[i]; });
	}

	template<typename FunctionType>
	bool Lookup(const wchar_t* key, const FunctionType& pfnFunction)
	{
		const auto found = m_argumentMap.find(key);
		if (found != m_argumentMap.end())
		{
			pfnFunction(found->second);
			return true;
		}

		return false;
	}

	bool GetInteger(const wchar_t* key, uint32_t& value)
	{
		return Lookup(key, [&value](std::wstring& val)
			{
				value = std::stoi(val);
			});
	}

	bool GetFloat(const wchar_t* key, float& value)
	{
		return Lookup(key, [&value](std::wstring& val)
			{
				value = (float)_wtof(val.c_str());
			});
	}

	bool GetString(const wchar_t* key, std::wstring& value)
	{
		return Lookup(key, [&value](std::wstring& val)
			{
				value = val;
			});
	}
}