#pragma once

#include <PhxResource/IResourceLoader.h>
#include <PhxRenderer/MaterialArchetypeResource.h>

namespace phx::renderer
{
	class MaterialArchetypeResourceHandler final : public phx::IResourceLoader
	{
	public:
		bool IsStale(AsyncResourceDescriptor const&, IVirtualFileSystem*) const override { return false; }
		LoaderStepResult Step(LoadContext& ctx) const override;

		static void SetForceShallowLoad(bool enable) { g_force_shallow_load = enable; }

	private:
		static bool LoadArchetype(LoadContext& ctx, RefCountPtr<MaterialArchetypeResource> mat_handle);

	private:
		inline static bool g_force_shallow_load = false;

	};
}