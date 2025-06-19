#pragma once

#include <PhxData/IAssetImporter.h>
#include <PhxData/IStreamingManager.h>
#include <PhxData/IVirtualFileSystem.h>

#include <PhxCore/StringHash.h>

#include <PhxWorld/WorldMetadata.def.h>

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
		phx::data::IStreamingManager* loader;
		std::vector<std::shared_ptr<phx::IBlob>> Blobs;
	};

	class GltfFileImporter final : public phx::data::IAssetImporter
	{
	public:
		phx::StringHash GetAssetTypeHash() const;
		void ImportAsync(phx::data::AssetManager* asset_manager, phx::RefCountPtr<phx::data::Asset> asset, std::string const& virtual_file_path) const;

	private:
		static void OnMainFileLoaded(phx::data::AsyncReadResult const& result, CgltfContext& context);
		static void LoadNodeRec(CgltfContext& ctx, cgltf_node const& gltfNode, phx::SceneBlueprint& scene, phx::NodeHandle parent);

		static void ProcessMesh(CgltfContext& context, phx::RefCountPtr<phx::renderer::MeshResource> resource);

	};
}

namespace phx::data
{
	template<>
	struct AssetImporterFileExtension<phxed::GltfFileImporter>
	{
		static constexpr const char* value = ".gltf";
	};

	template<>
	struct AssetImporterId<phxed::GltfFileImporter>
	{
		static constexpr phx::StringHash value = "gltf"_hash;
	};
}

