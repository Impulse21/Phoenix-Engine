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
		bool IsStale(AsyncResourceDescriptor const& resource_descriptor, IVirtualFileSystem* vfs) const override;
		void PrepareRequest(StreamingRequest& request, GenericHandle handle, IIoQueue* queue, AsyncResourceDescriptor const& resource_descriptor) const override;

		static void SetForceRecook(bool enable) { g_force_recook = enable; }

	private:
		static void CookPrefab(PrefabResourceHandle prefab_handle, AsyncResourceDescriptor const& gltf_resource_descriptor, void* file_data);
		static void LoadPrefab(std::ifstream& stream, PrefabResourceHandle prefab_handle);

		inline static bool g_force_recook = false;
	};
}
