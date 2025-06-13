#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/StringHash.h>
#include <string>

namespace phx::data
{
	struct Asset;

	template<typename T>
	struct AssetImporterFileExtension;

	template<typename T>
	struct AssetImporterId;

	class AssetManager;

	class IAssetImporter
	{
	public:
		virtual StringHash GetAssetTypeHash() const = 0;
		virtual void ImportAsync(AssetManager* asset_manager, RefCountPtr<Asset> asset, std::string const& virtual_file_path) const = 0;

		virtual ~IAssetImporter() = default;
	};
}