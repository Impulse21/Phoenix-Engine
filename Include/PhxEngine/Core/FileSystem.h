#pragma once

#include <filesystem>

namespace PhxEngine::Core
{
	class IBlob
	{
	public:
		[[nodiscard]] virtual const void* Data() const = 0;
		[[nodiscard]] virtual const size_t Size() const = 0;

		virtual ~IBlob() = default;
	};

	class Blob : public IBlob
	{
	public:
		Blob(void* data, size_t size);
		~Blob() override;

		const void* Data() const override { return this->m_data; }
		const size_t Size() const { return this->m_size; }

	private:
		void* m_data;
		size_t m_size;
	};

	class IFileSystem
	{
	public:
		virtual bool FileExists(std::filesystem::path const& filename) = 0;
		virtual std::unique_ptr<IBlob> ReadFile(std::filesystem::path const& filename) = 0;


		virtual ~IFileSystem() = default;
	};

	class NativeFileSystem : public IFileSystem
	{
	public:

		bool FileExists(std::filesystem::path const& filename) override;
		std::unique_ptr<IBlob> ReadFile(std::filesystem::path const& filename) override;
	};

	// A layer that represents some path in the underlying file system as an entire FS.
	// Effectively, just prepends the provided base path to every file name
	// and passes the requests to the underlying FS.
	class RelativeFileSystem : public IFileSystem
	{
	public:
		RelativeFileSystem(std::shared_ptr<IFileSystem> fs, const std::filesystem::path& basePath);

		[[nodiscard]] std::filesystem::path const& GetBasePath() const { return this->m_basePath; }

		bool FileExists(const std::filesystem::path& name) override;
		std::unique_ptr<IBlob> ReadFile(const std::filesystem::path& name) override;

	private:
		std::shared_ptr<IFileSystem> m_underlyingFS;
		std::filesystem::path m_basePath;
	};
}

