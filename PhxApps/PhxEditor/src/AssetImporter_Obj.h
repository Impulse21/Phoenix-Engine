#pragma once

#include <PhxCore/Span.h>

#include <PhxData/IAssetImporter.h>
#include <PhxData/IAsyncIOSystem.h>
#include <PhxData/IVirtualFileSystem.h>

#include <PhxCore/StringHash.h>
#include <PhxCore/RefCountPtr.h>

#include <fast_obj/fast_obj.h>

namespace phx
{
	namespace renderer
	{
		struct MeshResource;
	}
}

namespace phxed
{
	struct Mesh;
}
namespace phxed
{
	class ObjImporter final : public phx::data::IAssetImporter
	{
	public:
		phx::StringHash GetAssetTypeHash() const;
		void ImportAsync(phx::data::AssetManager* asset_manager, phx::RefCountPtr<phx::data::Asset> asset, std::string const& virtual_file_path) const;

	private:
		// shoudl be resued by gltf
		static Mesh GenerateMeshIndices(Mesh const& meshSrc, std::vector<uint32_t>& outRemap);
		static void OptimizeMesh(Mesh& mesh, std::vector<uint32_t>& remap);

		static std::string ProcessTexture(std::string const& base_viritual_path, const char* path);
		static void ProcessMesh(phx::renderer::MeshResource* resource, fastObjMesh* obj);

		static void PrintStatistics(Mesh const&);
	};
}

namespace phx::data
{
	template<>
	struct AssetImporterFileExtension<phxed::ObjImporter>
	{
		static constexpr const char* value = ".obj";
	};

	template<>
	struct AssetImporterId<phxed::ObjImporter>
	{
		static constexpr phx::StringHash value = "obj"_hash;
	};
}


