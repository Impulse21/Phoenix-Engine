#pragma once

#include <memory>
#include <string>
#include <filesystem>
#include <functional>
#include <vector>

#include <PhxEngine/Core/Span.h>

namespace PhxEngine
{
	namespace FileStatus
	{
		constexpr int OK = 0;
		constexpr int Failed = -1;
		constexpr int PathNotFound = -2;
		constexpr int NotImplemented = -3;
	}

	class IBlob
	{
	public:
		virtual ~IBlob() = default;

		[[nodiscard]] virtual const void* Data() const = 0;
		[[nodiscard]] virtual size_t Size() const = 0;

		static bool IsEmpty(IBlob const& blob)
		{
			return blob.Data() == nullptr || blob.Size() == 0;
		}
	};


	using EnumCallback = const std::function<void(std::string_view)>&;
	class IFileSystem
	{
	public:
		virtual ~IFileSystem() = default;

		virtual bool FileExists(std::filesystem::path const& name) = 0;
		virtual bool FolderExists(std::filesystem::path const& name) = 0;
		virtual std::unique_ptr<IBlob> ReadFile(std::filesystem::path const& name) = 0;
		virtual bool WriteFile(std::filesystem::path const& name, Span<char> Data) = 0;
		virtual int EnumerateFiles(const std::filesystem::path& path, const std::vector<std::string>& extensions, EnumCallback callback, bool allowDuplicates = false) = 0;
		virtual int EnumerateDirectories(const std::filesystem::path& path, EnumCallback callback, bool allowDuplicates = false) = 0;
	};

	class IRootFileSystem : public IFileSystem
	{
	public:
		virtual ~IRootFileSystem() = default;

		virtual void Mount(const std::filesystem::path& path, std::shared_ptr<IFileSystem> fs) = 0;
		virtual void Mount(const std::filesystem::path& path, const std::filesystem::path& nativePath) = 0;
		virtual bool Unmount(const std::filesystem::path& path) = 0;
		virtual int EnumerateFiles(const std::filesystem::path& path, const std::vector<std::string>& extensions, EnumCallback callback, bool allowDuplicates = false) = 0;
		virtual int EnumerateDirectories(const std::filesystem::path& path, EnumCallback callback, bool allowDuplicates = false) = 0;
	};


	class NativeFileSystem : public IFileSystem
	{
	public:
		bool FileExists(std::filesystem::path const& name) override;
		bool FolderExists(std::filesystem::path const& name) override;
		std::unique_ptr<IBlob> ReadFile(std::filesystem::path const& name) override;
		bool WriteFile(std::filesystem::path const& name, Span<char> Data) override;
		int EnumerateFiles(const std::filesystem::path& path, const std::vector<std::string>& extensions, EnumCallback callback, bool allowDuplicates = false) override;
		int EnumerateDirectories(const std::filesystem::path& path, EnumCallback callback, bool allowDuplicates = false) override;
	};

	class RelativeFileSystem : public IFileSystem
	{
	public:
		RelativeFileSystem(std::shared_ptr<IFileSystem> fs, const std::filesystem::path& baseBath);

		[[nodiscard]] std::filesystem::path const& GetBasePath() const { return this->m_basePath; }

		bool FileExists(std::filesystem::path const& name) override;
		bool FolderExists(std::filesystem::path const& name) override;
		std::unique_ptr<IBlob> ReadFile(std::filesystem::path const& name) override;
		bool WriteFile(std::filesystem::path const& name, Span<char> Data) override;
		int EnumerateFiles(const std::filesystem::path& path, const std::vector<std::string>& extensions, EnumCallback callback, bool allowDuplicates = false) override;
		int EnumerateDirectories(const std::filesystem::path& path, EnumCallback callback, bool allowDuplicates = false) override;

	private:
		std::shared_ptr<IFileSystem> m_underlyingFS;
		std::filesystem::path m_basePath;
	};

	class RootFileSystem : public IRootFileSystem
	{
	public:
		void Mount(const std::filesystem::path& path, std::shared_ptr<IFileSystem> fs) override;
		void Mount(const std::filesystem::path& path, const std::filesystem::path& nativePath) override;
		bool Unmount(const std::filesystem::path& path) override;

		bool FileExists(std::filesystem::path const& name) override;
		bool FolderExists(std::filesystem::path const& name) override;
		std::unique_ptr<IBlob> ReadFile(std::filesystem::path const& name) override;
		bool WriteFile(std::filesystem::path const& name, Span<char> Data) override;
		int EnumerateFiles(const std::filesystem::path& path, const std::vector<std::string>& extensions, EnumCallback callback, bool allowDuplicates = false) override;
		int EnumerateDirectories(const std::filesystem::path& path, EnumCallback callback, bool allowDuplicates = false) override;

	private:
		bool FindMountPoint(const std::filesystem::path& path, std::filesystem::path* pRelativePath, IFileSystem** ppFS);

	private:
		std::vector<std::pair<std::string, std::shared_ptr<IFileSystem>>> m_mountPoints;
	};

	namespace FileSystem
	{
		std::string GetFileNameWithoutExt(std::string const& path);
		std::string GetFileExt(std::string const& path); 
		std::string GetFileExt(std::string_view path);
	}

	namespace FileSystemFactory
	{
		std::unique_ptr<IFileSystem> CreateNativeFileSystem();
		std::unique_ptr<IFileSystem> CreateRelativeFileSystem(std::shared_ptr<IFileSystem> fs, const std::filesystem::path& baseBath);
		std::unique_ptr<IRootFileSystem> CreateRootFileSystem();
		std::unique_ptr<IBlob> CreateBlob(void* Data, size_t size);
	}
}