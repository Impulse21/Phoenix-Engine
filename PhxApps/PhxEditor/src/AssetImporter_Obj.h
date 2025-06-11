#pragma once

#include <PhxCore/Span.h>

#include <PhxData/IAssetImporter.h>
#include <PhxData/IAsyncIOSystem.h>
#include <PhxData/IVirtualFileSystem.h>

#include <PhxCore/StringHash.h>
#include <PhxCore/RefCountPtr.h>


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
		phx::RefCountPtr<phx::data::Asset> ImportAsync(phx::data::IVirtualFileSystem* vfs, phx::data::IAsyncIOSystem* loader, const char* virtual_file_path) const;

	private:
		static bool ParseObj(phx::SpanMutable<uint8_t> file_data, Mesh& meshData);

		// shoudl be resued by gltf
		static Mesh GenerateMeshIndices(Mesh const& meshSrc, std::vector<uint32_t>& outRemap);
		static void OptimizeMesh(Mesh& mesh, std::vector<uint32_t>& remap);
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


