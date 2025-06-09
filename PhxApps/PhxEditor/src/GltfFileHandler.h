#pragma once

#include <PhxResource/IResourceFileHandler.h>
#include <PhxData/IVirtualFileSystem.h>

#include <PhxCore/StringHash.h>

#include <PhxWorld/SceneBlueprint.h>

// -- forward declares ---
struct cgltf_data;
struct cgltf_mesh;
struct cgltf_node;

namespace phx
{
	namespace JobSystem
	{
		struct Barrier;
	}

	namespace renderer
	{
		struct MeshResource;
	}
}
namespace phxed
{
	struct CgltfContext
	{
		std::string virtual_file_path;
		phx::RefCountPtr<phx::SceneBlueprint> scene_resource;
		phx::JobSystem::Barrier* sub_resource_barrier;

		phx::Result<phx::data::AsyncResourceDescriptor> resource_descriptor;
		phx::data::IVirtualFileSystem* vfs;
		phx::data::IAsyncIOSystem* loader;
		std::vector<std::shared_ptr<phx::IBlob>> Blobs;
	};

	class GltfFileHandler final : public phx::ResourceFileHandler
	{
	public:
		phx::RefCountPtr<phx::Resource> LoadAsync(phx::data::IVirtualFileSystem* vfs, phx::data::IAsyncIOSystem* loader, const char* virtual_file_path) const;

	private:
		static void OnMainFileLoaded(phx::data::AsyncReadResult const& result, CgltfContext& context);
		static void LoadNodeRec(CgltfContext& ctx, cgltf_node const& gltfNode, phx::SceneBlueprint& scene, phx::NodeHandle parent);

		static void ProcessMesh(CgltfContext& context, phx::RefCountPtr<phx::renderer::MeshResource> resource);

	};
}

namespace phx
{
	template<>
	struct ResourceFileExtension<phxed::GltfFileHandler>
	{
		static constexpr const char* value = ".gltf";
	};

	template<>
	struct ResourceFileHandlerId<phxed::GltfFileHandler>
	{
		static constexpr phx::StringHash value = "gltf"_hash;
	};
}

