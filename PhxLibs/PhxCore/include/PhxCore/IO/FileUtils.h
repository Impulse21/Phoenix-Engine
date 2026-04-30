#pragma once

#include <string>
#include <filesystem>
#include <PhxCore/Platform/Platform.h>

namespace phx
{

	class IBlob
	{
	public:
		virtual ~IBlob() = default;

		[[nodiscard]] virtual const void* Data() const = 0;
		[[nodiscard]] virtual size_t Size() const = 0;

		static bool IsEmpty(IBlob* blob)
		{
			return blob == nullptr || blob->Data() == nullptr || blob->Size() == 0;
		}
	};

	class Blob : public IBlob
	{
	public:
		Blob(void* Data, size_t size)
			: m_data(Data)
			, m_size(size)
		{
		}
		Blob(size_t size)
			: m_data(malloc(size))
			, m_size(size)
		{
		}

		~Blob() override
		{
			if (m_data)
			{
				free(m_data);
				m_data = nullptr;
			}

			m_size = 0;
		}

		[[nodiscard]] void* Data() { return m_data; }
		[[nodiscard]] const void* Data() const override { return m_data; }
		[[nodiscard]] size_t Size() const override { return m_size; }

	private:
		void* m_data;
		size_t m_size;
	};

	inline std::string GetWorkingDirectory()
	{
		return std::filesystem::current_path().generic_string();
	}

	inline std::string GetDirectoryWithExecutable()
	{
		std::filesystem::path result = Platform::GetExectuablePath().ValueOr("");
		result = result.parent_path();

		return result.generic_string();
	}

	inline std::string GetFileNameWithoutExt(std::string const& path)
	{
		return std::filesystem::path(path).stem().generic_string();
	}
	inline std::string GetFileExt(std::string const& path)
	{
		return std::filesystem::path(path).extension().generic_string();
	}

	inline std::string GetDirectory(std::string const& path)
	{
		return std::filesystem::path(path).parent_path().generic_string();
	}

	inline bool FileExists(const std::string& path)
	{
		std::filesystem::path path_fs(path);
		return std::filesystem::exists(path_fs) && std::filesystem::is_regular_file(path_fs);
	}
	
	inline bool DirectoryExists(std::string const& path)
	{
		auto dir = std::filesystem::path(path).parent_path();
		return std::filesystem::exists(dir) && std::filesystem::is_directory(dir);
	}

	inline bool CreateDirectories(std::string const& path)
	{
		auto dir = std::filesystem::path(path).parent_path();
		return std::filesystem::create_directories(dir);
	}

	inline std::string NormalizePath(const std::string& path)
	{
    	std::string temp = path;
    	std::replace(temp.begin(), temp.end(), '\\', '/');

    	return temp;
	}

	inline std::string JoinPaths(const std::string& p1, const std::string& p2)
	{
		if (p1.empty())
			return p2;

		if (p2.empty())
			return p1;

		char sep = '/'; // Normalize to one separator type
#ifdef PHX_PLATFORM_WINDOWS
		// sep = '\\'; // Or keep '/' and let Windows handle it
#endif

		std::string result = p1;
		if (result.back() == '/' || result.back() == '\\')
		{
			if (p2.front() == '/' || p2.front() == '\\')
			{
				result += p2.substr(1);
			}
			else {
				result += p2;
			}
		}
		else
		{
			if (p2.front() == '/' || p2.front() == '\\')
			{
				result += p2;
			}
			else {
				result += sep;
				result += p2;
			}
		}

		// Replace all '\\' with '/' for internal consistency if desired
		std::replace(result.begin(), result.end(), '\\', '/');
		return result;
	}
}