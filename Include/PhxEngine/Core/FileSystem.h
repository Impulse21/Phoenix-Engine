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
		virtual std::shared_ptr<IBlob> ReadFile(std::filesystem::path const& filename) = 0;


		virtual ~IFileSystem() = default;
	};

	class NativeFileSystem : public IFileSystem
	{
	public:

		bool FileExists(std::filesystem::path const& filename) override;
		std::shared_ptr<IBlob> ReadFile(std::filesystem::path const& filename) override;
	};
}

