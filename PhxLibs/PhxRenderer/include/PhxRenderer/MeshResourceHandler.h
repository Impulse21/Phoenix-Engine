#pragma once

#include <PhxResource/IResourceLoader.h>
#include <PhxRenderer/MeshResource.h>

namespace phx::renderer
{
	class MeshResourceHandler final : public phx::IResourceLoader
	{
	public:
		bool IsStale(AsyncResourceDescriptor const&, IRootFileSystem*) const override { return false; }
		LoaderStepResult Step(LoadContext& ctx) const override;

	private:
	};
}