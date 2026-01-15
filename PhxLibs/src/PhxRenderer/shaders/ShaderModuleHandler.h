#pragma once

#include <PhxResource/IResourceLoader.h>
#include <PhxRenderer/shaders/ShaderModuleResource.h>

namespace phx::renderer
{
	class ShaderModuleHandler final : public phx::IResourceLoader
	{
	public:
		bool IsStale(AsyncResourceDescriptor const&, IVirtualFileSystem*) const override { return false; }
		LoaderStepResult Step(LoadContext& ctx) const override;	

	private:

	};
}

