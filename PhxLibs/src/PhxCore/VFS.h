#pragma once

#include <memory>
#include <string>
#include <filesystem>
#include <functional>
#include <vector>
#include <PhxCore/Handle.h>
#include "PhxCore/Span.h"

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

		~Blob() override
		{
			if (this->m_data)
			{
				free(this->m_data);
				this->m_data = nullptr;
			}

			this->m_size = 0;
		}

		[[nodiscard]] const void* Data() const override { return this->m_data; }
		[[nodiscard]] size_t Size() const override { return this->m_size; }

	private:
		void* m_data;
		size_t m_size;
	};

	class IFileSystem
	{
	public:
		virtual ~IFileSystem() = default;

		virtual bool FileExists(std::filesystem::path const& name) = 0;
		virtual bool FolderExists(std::filesystem::path const& name) = 0;
		virtual std::unique_ptr<IBlob> ReadFile(std::filesystem::path const& name) = 0;

		virtual bool WriteFile(std::filesystem::path const& name, Span<char> Data) = 0;
		bool WriteFile(std::filesystem::path const& name, IBlob* blob)
		{
			return WriteFile(name, Span<char>(reinterpret_cast<const char*>(blob->Data()), blob->Size()));
		}

		virtual std::filesystem::path ResolvePath(std::filesystem::path const& name) = 0;
	};

	class IRootFileSystem : public IFileSystem
	{
	public:
		inline static IRootFileSystem* Ptr = nullptr;

	public:
		virtual ~IRootFileSystem() override = default;

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

	namespace VFS
	{
		std::filesystem::path GetDirectoryWithExecutable();
		std::string GetFileNameWithoutExt(std::string const& path);
		std::string GetFileExt(std::string const& path);

	}


	class NativeFileSystem final : public IFileSystem
	{
	public:
		bool FileExists(std::filesystem::path const& name) override;
		bool FolderExists(std::filesystem::path const& name) override;
		std::unique_ptr<IBlob> ReadFile(std::filesystem::path const& name) override;
		bool WriteFile(std::filesystem::path const& name, Span<char> Data) override;

		std::filesystem::path ResolvePath(std::filesystem::path const& name) override
		{
			return name;
		}
	};

	class RelativeFileSystem final : public IFileSystem
	{
	public:
		RelativeFileSystem(std::shared_ptr<IFileSystem> fs, const std::filesystem::path& baseBath);

		[[nodiscard]] std::filesystem::path const& GetBasePath() const { return this->m_basePath; }

		bool FileExists(std::filesystem::path const& name) override;
		bool FolderExists(std::filesystem::path const& name) override;
		std::unique_ptr<IBlob> ReadFile(std::filesystem::path const& name) override;
		bool WriteFile(std::filesystem::path const& name, Span<char> Data) override;

		std::filesystem::path ResolvePath(std::filesystem::path const& name) override
		{
			return this->m_basePath / name.relative_path();
		}

	private:
		std::shared_ptr<IFileSystem> m_underlyingFS;
		std::filesystem::path m_basePath;
	};

	class RootFileSystem final : public IRootFileSystem
	{
	public:
		void Mount(const std::filesystem::path& path, std::shared_ptr<IFileSystem> fs) override;
		void Mount(const std::filesystem::path& path, const std::filesystem::path& nativePath) override;
		bool Unmount(const std::filesystem::path& path) override;

		bool FileExists(std::filesystem::path const& name) override;
		bool FolderExists(std::filesystem::path const& name) override;
		std::unique_ptr<IBlob> ReadFile(std::filesystem::path const& name) override;
		bool WriteFile(std::filesystem::path const& name, Span<char> Data) override;
		std::filesystem::path ResolvePath(std::filesystem::path const& name) override;

	protected:
		bool FindMountPoint(const std::filesystem::path& path, std::filesystem::path* pRelativePath, IFileSystem** ppFS);

	private:
		std::vector<std::pair<std::string, std::shared_ptr<IFileSystem>>> m_mountPoints;
	};
}