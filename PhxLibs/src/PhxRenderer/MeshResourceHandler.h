#pragma once

#include <PhxResource/IResource.h>
#include <PhxResource/IResourceHandler.h>

namespace phx::renderer
{
	class MeshResourceHandler final : public phx::IResourceHandler
	{
	public:
		RefCountPtr<IResource> Load(
			std::shared_ptr<IAssetStreamer> const& assetStreamer,
			StreamFileHandle filehandle,
			PakFileFormat::AssetEntry const& assetEntry) const override;
	};
}

namespace phx
{
	template<>
	struct ResourceExtension<renderer::MeshResourceHandler>
	{
		static constexpr const char* value = ".phxmsh";
	};

	template<>
	struct ResourceHandlerId<renderer::MeshResourceHandler>
	{
		static constexpr phx::StringHash value = "phxmsh";
	};
}

