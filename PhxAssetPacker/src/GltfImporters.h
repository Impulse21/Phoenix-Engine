#pragma once

#include <memory>

#include "MeshResourceCompiler.h"
#include "PhxWorld/World.h"
#include "PhxWorld/Entity.h"

// -- forward declares ---
struct cgltf_data;
struct cgltf_mesh;
struct cgltf_node;

namespace phx
{
	template <typename T>
	class GltfImporter
	{
	public:
		template <typename... TArgs>
		static bool Import(TArgs&&... args)
		{
			T importer(std::forward<TArgs>(args)...);
			return importer.ImportImpl();
		}

	};

	class GltfMeshImporter : public GltfImporter<GltfMeshImporter>
	{
	public:
		GltfMeshImporter(cgltf_data* gltfData, std::vector<MeshData>& outMeshData)
			: m_gltfData(gltfData)
			, m_out(outMeshData)
		{}

		 bool ImportImpl();

	private:
		cgltf_data* m_gltfData;
		std::vector<MeshData>& m_out;
	};


	class GltfWorldImporter : public GltfImporter<GltfWorldImporter>
	{
	public:
		GltfWorldImporter(cgltf_data* gltfData, phx::World& world)
			: m_gltfData(gltfData)
			, m_out(world)
		{
		}

		bool ImportImpl();

	private:
		void LoadNodeRec(cgltf_node const& gltfNode, phx::Entity parent);

	private:
		cgltf_data* m_gltfData;
		phx::World& m_out;
	};

}

