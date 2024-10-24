#pragma once

#pragma once

#include <memory>
#include <string>
#include <filesystem>
#include <functional>
#include <vector>

#include "phxSpan.h"

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

	class IFileSystem
	{
	public:
		virtual ~IFileSystem() = default;

		virtual bool FileExists(std::filesystem::path const& name) = 0;
		virtual bool FolderExists(std::filesystem::path const& name) = 0;
		virtual std::unique_ptr<IBlob> ReadFile(std::filesystem::path const& name) = 0;
		virtual bool WriteFile(std::filesystem::path const& name, Span<char> Data) = 0;
	};

	class IRootFileSystem : public IFileSystem
	{
	public:
		virtual ~IRootFileSystem() = default;

		virtual void Mount(const std::filesystem::path& path, std::shared_ptr<IFileSystem> fs) = 0;
		virtual void Mount(const std::filesystem::path& path, const std::filesystem::path& nativePath) = 0;
		virtual bool Unmount(const std::filesystem::path& path) = 0;
	};

	namespace FileSystemFactory
	{
		std::unique_ptr<IFileSystem> CreateNativeFileSystem();
		std::unique_ptr<IFileSystem> CreateRelativeFileSystem(std::shared_ptr<IFileSystem> fs, const std::filesystem::path& baseBath);
		std::unique_ptr<IRootFileSystem> CreateRootFileSystem();
		std::unique_ptr<IBlob> CreateBlob(void* Data, size_t size);
	}

	namespace FS
	{
		inline static IRootFileSystem* RootPtr = nullptr;
		
		std::filesystem::path GetDirectoryWithExecutable();
	}
}