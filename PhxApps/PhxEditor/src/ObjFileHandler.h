#pragma once

#include <PhxCore/Span.h>
#include <PhxResource/IResourceFileHandler.h>
#include <PhxData/IVirtualFileSystem.h>
#include <PhxCore/StringHash.h>


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
	class ObjFileHandler final : public phx::ResourceFileHandler
	{
	public:

		phx::RefCountPtr<phx::Resource> LoadAsync(phx::data::IVirtualFileSystem* vfs, phx::data::IAsyncIOSystem* loader, const char* virtual_file_path) const;

	private:
		static bool ParseObj(phx::SpanMutable<uint8_t> file_data, Mesh& meshData);

		// shoudl be resued by gltf
		static Mesh GenerateMeshIndices(Mesh const& meshSrc, std::vector<uint32_t>& outRemap);
		static void OptimizeMesh(Mesh& mesh, std::vector<uint32_t>& remap);
		static void PrintStatistics(Mesh const&);
	};
}

namespace phx
{
	template<>
	struct ResourceFileExtension<phxed::ObjFileHandler>
	{
		static constexpr const char* value = ".obj";
	};

	template<>
	struct ResourceFileHandlerId<phxed::ObjFileHandler>
	{
		static constexpr phx::StringHash value = "obj"_hash;
	};
}


