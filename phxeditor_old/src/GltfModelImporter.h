#pragma once


#include "ModelImporter.h"
#include "phxMath.h"
#include <unordered_map>
#include <vector>
#include <memory>

struct cgltf_material;
struct cgltf_mesh;
struct cgltf_texture;
struct cgltf_data;
struct cgltf_node;

namespace phxed
{
	struct GraphNode // 96 bytes
	{
		DirectX::XMMATRIX Xform;
		DirectX::XMFLOAT4 Dotation;
		DirectX::XMFLOAT3 scale;

		uint32_t matrixIdx : 28;
		uint32_t hasSibling : 1;
		uint32_t hasChildren : 1;
		uint32_t staleMatrix : 1;
		uint32_t skeletonRoot : 1;
	};

	class IBlob;
	class IFileSystem;
	struct CgltfContext
	{
		IFileSystem* FileSystem;
		std::vector<std::shared_ptr<IBlob>> Blobs;
	};

	class phxModelImporterGltf final : public ModelImporter
	{
	public:
		phxModelImporterGltf(IFileSystem* fs)
			: m_fs(fs)
		{}
		~phxModelImporterGltf() override = default;

		bool Import(std::string const& filename, ModelData& outModel) override;

	private:
		void BuildMaterials(ModelData& outModel);
		size_t WalkGraphRec(
			std::vector<GraphNode>& sceneGraph,
			phx::math::Sphere& modelBSphere,
			phx::math::AABB& modelBBox,
			std::vector<Mesh*>& meshList,
			std::vector<uint8_t>& bufferMemory,
			cgltf_node** siblings,
			size_t numSiblings,
			size_t curPos,
			DirectX::XMMATRIX const& xform);

		void CompileMesh(
			std::vector<Mesh*>& meshList,
			std::vector<uint8_t>& bufferMemory,
			cgltf_mesh& srcMesh,
			size_t matrixIdx,
			const DirectX::XMMATRIX& localToObject,
			size_t skinIndex,
			phx::math::Sphere& boundingSphere,
			phx::math::AABB& boundingBox);

	private:
		IFileSystem* m_fs = nullptr;
		std::unordered_map<cgltf_mesh*, size_t> m_meshIndexLut;
		std::unordered_map<cgltf_material*, size_t> m_materialIndexLut;
		std::unordered_map<cgltf_texture*, size_t> m_textureIndexLut;
		cgltf_data* m_gltfData;
		CgltfContext m_cgltfContext;
	};
}

