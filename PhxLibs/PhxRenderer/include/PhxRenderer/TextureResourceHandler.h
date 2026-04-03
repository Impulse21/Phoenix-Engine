#pragma once

#include <PhxResource/IResourceLoader.h>
#include <PhxRenderer/TextureResource.h>

namespace phx::renderer
{
	class TextureResourceHandler final : public phx::IResourceLoader
	{
	public:
		bool IsStale(AsyncResourceDescriptor const&, IVirtualFileSystem*) const override { return false; }
		LoaderStepResult Step(LoadContext& ctx) const override;

		static bool RhiResourcesCreated(RefCountPtr<TextureResource> tex_res);
		
	private:
	};
}
