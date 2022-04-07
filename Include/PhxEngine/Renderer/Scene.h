#pragma once

#include <sstream>

#include <PhxEngine/Core/FileSystem.h>
#include <PhxEngine/Renderer/TextureCache.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Renderer/SceneDataTypes.h>
#include <PhxEngine/Renderer/SceneNodes.h>

#include <PhxEngine/Renderer/ShaderStructures.h>

#include <PhxEngine/Ecs/Ecs.h>
#include <PhxEngine/Renderer/SceneComponents.h>

namespace PhxEngine::Renderer
{
	class SceneGraph
	{
	public:
		// Set Node Data
		SceneGraph();
		
		std::shared_ptr<Node> GetRootNode() { return this->m_rootNode; }

		template <class TNode>
		std::shared_ptr<TNode> CreateNode() = delete;

		template<>
		std::shared_ptr<MeshInstanceNode> CreateNode()
		{
			auto node = this->m_meshInstanceNodes.emplace_back(std::make_shared<MeshInstanceNode>());
			node->SetInstanceIndex(this->m_meshInstanceNodes.size() - 1);
			return node;
		}

		template<>
		std::shared_ptr<LightNode> CreateNode()
		{
			return this->m_lightNodes.emplace_back(std::make_shared<LightNode>());
		}

		template<>
		std::shared_ptr<PerspectiveCameraNode> CreateNode()
		{
			auto perspectiveCamera = std::make_shared<PerspectiveCameraNode>();
			this->m_cameraNodes.emplace_back(perspectiveCamera);
			return perspectiveCamera;
		}

		std::shared_ptr<Material> CreateMaterial()
		{
			return this->m_materials.emplace_back(std::make_shared<Material>());
		}


		std::shared_ptr<Mesh> CreateMesh()
		{
			auto mesh = this->m_meshes.emplace_back(std::make_shared<Mesh>());
			mesh->Buffers = std::make_shared<MeshBuffers>();
			return mesh;
		}

		void Refresh();

		void AttachNode(std::shared_ptr<Node> parent, std::shared_ptr<Node> child);

		const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const { return this->m_meshes; }
		const std::vector<std::shared_ptr<Material>>& GetMaterials() const { return this->m_materials; }
		const std::vector<std::shared_ptr<MeshInstanceNode>>& GetMeshInstanceNodes() const { return this->m_meshInstanceNodes; }

	private:
		friend class Node;

		std::shared_ptr<Node> m_rootNode;
		std::vector<std::shared_ptr<Material>> m_materials;
		std::vector<std::shared_ptr<Mesh>> m_meshes;

		std::vector<std::shared_ptr<MeshInstanceNode>> m_meshInstanceNodes;
		std::vector<std::shared_ptr<LightNode>> m_lightNodes;
		std::vector<std::shared_ptr<CameraNode>> m_cameraNodes;
		bool m_isStructureDirty = false;
	};

	class Scene
	{
	public:
		Scene(
			std::shared_ptr<Core::IFileSystem> fileSystem,
			std::shared_ptr<TextureCache> textureCache,
			RHI::IGraphicsDevice* graphicsDevice);

		void SetSceneGraph(std::shared_ptr<SceneGraph> sceneGraph) { this->m_sceneGraph = sceneGraph; }
		std::shared_ptr<SceneGraph> GetSceneGraph() { return this->m_sceneGraph; }

		void RefreshGpuBuffers(RHI::ICommandList* commandList);

		void SetMainCamera(std::shared_ptr<CameraNode> mainCamera) { this->m_mainCamera = mainCamera; }
		std::shared_ptr<CameraNode> GetMainCamera() { return this->m_mainCamera; }

		RHI::BufferHandle GetGeometryBuffer() { return this->m_geometryBuffer; }
		RHI::BufferHandle GetInstanceBuffer() { return this->m_instanceBuffer; }
		RHI::BufferHandle GetMaterialBuffer() { return this->m_materialBuffer; }

		const std::vector<MaterialData>& GetMaterialData() const { return this->m_resources->MaterialData; }
		const std::vector<GeometryData>& GetGeometryData() const { return this->m_resources->GeometryData; }
		const std::vector<MeshInstanceData> GetMeshInstanceData() const { return this->m_resources->MeshInstanceData; }

	private:
		void CreateMeshBuffers(RHI::ICommandList* commandList);
		RHI::BufferHandle CreateGeometryBuffer();
		RHI::BufferHandle CreateMaterialBuffer();
		RHI::BufferHandle CreateInstanceBuffer();

		void UpdateGeometryData(std::shared_ptr<Mesh> const& mesh);
		void WriteGeometryBuffer(RHI::ICommandList* commandList);

		void UpdateMaterialData(std::shared_ptr<Material> const& material);
		void WriteMaterialBuffer(RHI::ICommandList* commandList);

		void UpdateInstanceData(std::shared_ptr<MeshInstanceNode> const& meshInstanceNode);
		void WriteInstanceBuffer(RHI::ICommandList* commandList);

	private:
		struct Resources
		{
			std::vector<MaterialData> MaterialData;
			std::vector<GeometryData> GeometryData;
			std::vector<MeshInstanceData> MeshInstanceData;
		};

		std::shared_ptr<CameraNode> m_mainCamera;

		std::unique_ptr<Resources> m_resources;
		RHI::BufferHandle m_geometryBuffer;
		RHI::BufferHandle m_materialBuffer;
		RHI::BufferHandle m_instanceBuffer;

		std::shared_ptr<Core::IFileSystem> m_fs;
		std::shared_ptr<TextureCache> m_textureCache;
		std::shared_ptr<SceneGraph> m_sceneGraph;
		RHI::IGraphicsDevice* m_graphicsDevice;
	};


	namespace New
	{
		class Scene
		{
		public:
			Scene() = default;
			~Scene() = default;

			static CameraComponent& GetGlobalCamera();

			// Helper
			ECS::Entity EntityCreateMeshInstance(std::string const& name);
			ECS::Entity EntityCreateCamera(
				std::string const& name,
				float width,
				float height,
				float nearPlane = 0.01f,
				float farPlane = 1000.0f,
				float fov = DirectX::XM_PIDIV4);

			ECS::Entity EntityCreateMaterial(std::string const& name);
			ECS::Entity EntityCreateMesh(std::string const& name);
			ECS::Entity EntityCreateLight(std::string const& name);

			ECS::Entity CreateCubeMeshEntity(std::string const& name, ECS::Entity mtlIDmtl, float size, bool rhsCoords);

			void ComponentAttach(ECS::Entity entity, ECS::Entity parent, bool childInLocalSpace = false);
			void ComponentDetach(ECS::Entity entity);
			void ComponentDetachChildren(ECS::Entity parent);

			void RefreshGpuBuffers(RHI::IGraphicsDevice* graphicsDevice, RHI::CommandListHandle commandList);

			void PopulateShaderSceneData(Shader::SceneData& sceneData);

			// -- System functions ---
		public:
			void UpdateTansformsSystem();
			void UpdateHierarchySystem();

			// -- Feilds ---
		public:
			NameComponentStore Names;
			TransformComponentStore Transforms;
			HierarchyComponentStore Hierarchy;
			MaterialComponentStore Materials;
			MeshComponentStore Meshes;
			MeshInstanceComponentStore MeshInstances;

			LightComponentStore Lights;
			CameraComponentStore Cameras;

		public:
			PhxEngine::RHI::TextureHandle SkyboxTexture;
			PhxEngine::RHI::TextureHandle IrradanceMap;
			PhxEngine::RHI::TextureHandle PrefilteredMap;
			PhxEngine::RHI::TextureHandle BrdfLUT;

			RHI::BufferHandle GeometryGpuBuffer;
			RHI::BufferHandle MeshGpuBuffer;
			RHI::BufferHandle MaterialBuffer;

		private:
			std::vector<Shader::MaterialData> m_materialShaderData;
			std::vector<Shader::Geometry> m_geometryShaderData;
		};

		void PrintScene(Scene const& scene, std::stringstream& stream);
	}

	namespace SceneGraphHelpers
	{
		extern void InitializeSphereMesh(
			std::shared_ptr<Mesh> outMesh,
			std::shared_ptr<Material> material,
			float diameter = 1,
			size_t tessellation = 16,
			bool rhcoords = false);


		extern void InitializePlaneMesh(
			std::shared_ptr<Mesh> outMesh,
			std::shared_ptr<Material> material,
			float width,
			float height,
			bool rhcoords);

		void InitalizeCubeMesh(
			std::shared_ptr<Mesh> outMesh,
			std::shared_ptr<Material> material,
			float size,
			bool rhsCoord);
	}
}