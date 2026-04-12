#pragma once

#include <unordered_map>

#include <PhxCore/Span.h>

#include <PhxResource/IResourceLoader.h>

#include <PhxWorld/PrefabResource.h>

struct cgltf_mesh;

namespace phx
{
	struct AsyncResourceDescriptor;

	class GltfPrefabLoader final : public phx::IResourceLoader
	{
	public:
		bool IsStale(AsyncResourceDescriptor const& resource_descriptor, phx::IRootFileSystem* vfs) const override;
		LoaderStepResult Step(LoadContext& ctx) const override;

		static void SetForceRecook(bool enable) { g_force_recook = enable; }
		static void SetForceShallowLoad(bool enable) { g_force_shallow_load = enable; }

	private:
		static void CookPrefab(RefCountPtr<PrefabResource> prefab_handle, AsyncResourceDescriptor const& gltf_resource_descriptor, void* file_data);
		static void LoadPrefab(LoadContext& ctx, RefCountPtr<PrefabResource> prefab_handle);

		inline static bool g_force_recook = false;
		inline static bool g_force_shallow_load = false;
	};
}
