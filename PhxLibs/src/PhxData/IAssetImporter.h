#pragma once

namespace phx::data
{
	struct Asset;

	template<typename T>
	struct AssetImporterFileExtension;

	template<typename T>
	struct AssetImporterId;

	class IVirtualFileSystem;
	class IAsyncIOSystem;

	class IAssetImporter
	{
	public:
		virtual RefCountPtr<Asset> ImportAsync(IVirtualFileSystem* vfs, IAsyncIOSystem* loader, const char* virtual_file_path) const = 0;
	};
}