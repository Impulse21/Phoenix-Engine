#pragma once

#include <memory>

#include "MeshResourceCompiler.h"

struct cgltf_data;
struct cgltf_mesh;

namespace phx
{
	class GltfMeshImporter
	{
	public:
		static std::vector<MeshData> Import(cgltf_data* gltfData)
		{
			GltfMeshImporter importer(gltfData);
			return importer.Import();
		}

	private:
		GltfMeshImporter(cgltf_data* gltfData)
			: m_gltfData(gltfData)
		{}

		std::vector<MeshData> Import();

	private:
		cgltf_data* m_gltfData;
	};
}

