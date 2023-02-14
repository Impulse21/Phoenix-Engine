#pragma once

#include <string>
#include <fstream>
#include <filesystem>

#ifdef _WIN32
#include <stringapiset.h>
#endif // _WIN32

namespace PhxEngine::Core
{
	namespace Helpers
	{
		inline void StringConvert(std::string const& from, std::wstring& to)
		{
#ifdef _WIN32
			int num = MultiByteToWideChar(CP_UTF8, 0, from.c_str(), -1, NULL, 0);
			if (num > 0)
			{
				to.resize(size_t(num) - 1);
				MultiByteToWideChar(CP_UTF8, 0, from.c_str(), -1, &to[0], num);
			}
#else
			std::wstring_convert<std::codecvt_utf8<wchar_t>> cv;
			to = cv.from_bytes(from);
#endif // _WIN32

		}

		inline void StringConvert(std::wstring const& from, std::string& to)
		{
#ifdef _WIN32
			int num = WideCharToMultiByte(CP_UTF8, 0, from.c_str(), -1, NULL, 0, NULL, NULL);
			if (num > 0)
			{
				to.resize(size_t(num) - 1);
				WideCharToMultiByte(CP_UTF8, 0, from.c_str(), -1, &to[0], num, NULL, NULL);
			}
#else
			std::wstring_convert<std::codecvt_utf8<wchar_t>> cv;
			to = cv.to_bytes(from);
#endif // _WIN32
		}

		inline void DirectoryCreate(std::filesystem::path const& path)
		{
			std::filesystem::create_directories(path);
		}

		inline bool FileRead(std::string const& file, std::vector<uint8_t>& Data)
		{
			std::ifstream fStream(file, std::ios::binary | std::ios::ate);

			if (fStream.is_open())
			{
				const size_t dataSize = fStream.tellg();
				fStream.seekg(0, fStream.beg);
				Data.resize(dataSize);
				fStream.read((char*)Data.data(), dataSize);
				fStream.close();
				return true;
			}

			return false;
		}

		inline bool FileWrite(const std::string& fileName, const uint8_t* Data, size_t size)
		{
			if (size <= 0)
			{
				return false;
			}

			std::ofstream file(fileName, std::ios::binary | std::ios::trunc);
			if (file.is_open())
			{
				file.write((const char*)Data, (std::streamsize)size);
				file.close();
				return true;
			}

			return false;
		}

		constexpr uint32_t AlignTo(uint32_t value, uint32_t alignment)
		{
			if (alignment == 0)
			{
				return value;
			}

			return ((value + alignment - 1) / alignment) * alignment;
		}

		constexpr uint64_t AlignTo(uint64_t value, uint64_t alignment)
		{
			if (alignment == 0)
			{
				return value;
			}

			return ((value + alignment - 1) / alignment) * alignment;
		}
	}
}