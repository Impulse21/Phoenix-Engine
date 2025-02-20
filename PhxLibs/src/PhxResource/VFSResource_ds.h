#pragma once

#include "VFSResource.h"
#include "PhxCore/Pool.h"

#include <dstorage.h>

#include <wrl.h>

namespace phx
{
	struct FileDS
	{
		Microsoft::WRL::ComPtr<IDStorageFile> DsFile;
		BY_HANDLE_FILE_INFORMATION FileInfo = {};
		Microsoft::WRL::ComPtr<IDStorageStatusArray> StatusArray;
	};

	class DSResourceFileSystem final : public IResourceFileSystem
	{
	public:
		DSResourceFileSystem();
		~DSResourceFileSystem() override = default;

		FileHandle Open(std::filesystem::path const& path) override;
		void Close(FileHandle handle) override;

		void EnqueueRead(FileHandle handle, uint64_t offset, uint64_t size, void* dest, RequestCallbackFunc&& callback) override;
		std::unique_ptr<IBlob> EnqueueReadBlob(FileHandle handle, uint64_t offset, uint64_t size, RequestCallbackFunc&& callback) override;

		void SubmitRequests() override;

	public:
		void Mount(const std::filesystem::path& path, std::shared_ptr<IFileSystem> fs) override
		{
			m_rootFs->Mount(path, fs);
		}

		void Mount(const std::filesystem::path& path, const std::filesystem::path& nativePath) override
		{
			m_rootFs->Mount(path, nativePath);
		}

		bool Unmount(const std::filesystem::path& path) override
		{
			m_rootFs->Unmount(path);
		}

		bool FileExists(std::filesystem::path const& name) override
		{
			m_rootFs->FileExists(name);
		}

		bool FolderExists(std::filesystem::path const& name) override
		{
			m_rootFs->FolderExists(name);
		}

		std::unique_ptr<IBlob> ReadFile(std::filesystem::path const& name) override
		{
			m_rootFs->ReadFile(name);
		}

		bool WriteFile(std::filesystem::path const& name, Span<char> Data) override
		{
			m_rootFs->WriteFile(name, Data);
		}

		
	private:
		std::unique_ptr<IRootFileSystem> m_rootFs;
		phx::ResourcePool<File, FileDS> m_filePool;
	};
}

