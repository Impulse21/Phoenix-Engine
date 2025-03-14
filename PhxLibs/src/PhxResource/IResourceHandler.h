#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/StringHash.h>

#include <PhxResource\IAssetStreamer.h>
#include <PhxResource\PakFileFormat.h>
#include <memory>
#include <filesystem>

namespace phx
{
	template<typename T>
	struct ResourceExtension;

	template<typename T>
	struct ResourceHandlerId;

	class IResource;
	class IResourceHandler
	{
	public:
		virtual RefCountPtr<IResource> Load(
			std::shared_ptr<IAssetStreamer> const& assetStreamer,
			StreamFileHandle filehandle,
			PakFileFormat::AssetEntry const& assetEntry) const = 0;

		virtual ~IResourceHandler() = default;
	};
}