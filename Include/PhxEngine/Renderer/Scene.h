#pragma once

#include <PhxEngine/Core/FileSystem.h>
#include <PhxEngine/Renderer/TextureCache.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Renderer/SceneDataTypes.h>
#include <PhxEngine/Renderer/SceneNodes.h>

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
			return this->m_meshInstanceNodes.emplace_back();
		}

		template<>
		std::shared_ptr<LightNode> CreateNode()
		{
			return this->m_lightNodes.emplace_back();
		}

		template<>
		std::shared_ptr<CameraNode> CreateNode()
		{
			return this->m_cameraNodes.emplace_back();
		}

		std::shared_ptr<Material> CreateMaterial()
		{
			return this->m_materials.emplace_back();
		}

	private:
		friend class Node;

		std::shared_ptr<Node> m_rootNode;
		std::vector<std::shared_ptr<Material>> m_materials;
		std::vector<std::shared_ptr<Mesh>> m_mesh;

		std::vector<std::shared_ptr<MeshInstanceNode>> m_meshInstanceNodes;
		std::vector<std::shared_ptr<LightNode>> m_lightNodes;
		std::vector<std::shared_ptr<CameraNode>> m_cameraNodes;
	};

	class Scene
	{
	public:
		Scene(
			std::shared_ptr<Core::IFileSystem> fileSystem,
			std::shared_ptr<TextureCache> textureCache,
			RHI::IGraphicsDevice* graphicsDevice);


		void SetSceneGraph(std::shared_ptr<SceneGraph> sceneGraph) { this->m_sceneGraph = sceneGraph; }

		uint64_t ConstructGpuData();

	private:
		std::shared_ptr<Core::IFileSystem> m_fs;
		std::shared_ptr<TextureCache> m_textureCache;
		std::shared_ptr<SceneGraph> m_sceneGraph;
		RHI::IGraphicsDevice* m_graphicsDevice;
	};
}