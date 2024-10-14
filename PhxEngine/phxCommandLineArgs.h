#pragma once

namespace phx
{
namespace CommandLineArgs
{
	void Initialize(int argc, wchar_t** argv);
	bool GetInteger(const wchar_t* key, uint32_t& value);
	bool GetFloat(const wchar_t* key, float& value);
	bool GetString(const wchar_t* key, std::wstring& value);
	bool HasFlag(const wchar_t* key);
}
}