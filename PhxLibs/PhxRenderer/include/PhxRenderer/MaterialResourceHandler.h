#pragma once

#include <PhxResource/IResourceLoader.h>
#include <PhxRenderer/MaterialResource.h>

namespace phx::renderer
{
	class MaterialResourceHandler final : public phx::IResourceLoader
	{
	public:
		bool IsStale(AsyncResourceDescriptor const&, IVirtualFileSystem*) const override { return false; }
		LoaderStepResult Step(LoadContext& ctx) const override;

		static void SetForceShallowLoad(bool enable) { g_force_shallow_load = enable; }

	private:
		static void LoadMaterial(LoadContext& ctx, RefCountPtr<MaterialResource> mat_handle);

	private:
		inline static bool g_force_shallow_load = false;

	};
}