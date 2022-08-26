#pragma once

#include <sstream>

#include "PhxEngine/Systems/Ecs.h"
#include "PhxEngine/Scene/SceneComponents.h"

#include "PhxEngine/Graphics/RHI/PhxRHI.h"
#include <PhxEngine/Scene/AssetStore.h>

#include <entt.hpp>

namespace PhxEngine::Scene
{
	class Entity;

	namespace New
	{
		class Scene
		{
			friend Entity;
		public:

			Entity CreateEntity(std::string const& name = std::string());
			void DestroyEntity(Entity entity);

			template<typename... Components>
			auto GetAllEntitiesWith()
			{
				return this->m_registry.view<Components...>();
			}

			entt::registry& GetRegistry() { return this->m_registry; }

			AssetStore& GetAssetStore() { return this->m_assetStore; }

		private:
			AssetStore m_assetStore;
			entt::registry m_registry;

		};
	}


	namespace Legacy
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
				std::string const& name);

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
			ECS::Entity CreateSphereMeshEntity(std::string const& name, ECS::Entity mtlId, float diameter, size_t tessellation, bool rhsCoords);

			void ComponentAttach(ECS::Entity entity, ECS::Entity parent, bool childInLocalSpace = false);
			void ComponentDetach(ECS::Entity entity);
			void ComponentDetachChildren(ECS::Entity parent);


			void RunMeshInstanceUpdateSystem();

			// -- System functions ---
		public:
			void UpdateTansformsSystem();
			void UpdateHierarchySystem();
			void UpdateLightsSystem();

			// GPU Related functions
		public:
			void RefreshGpuBuffers(RHI::IGraphicsDevice* graphicsDevice, RHI::CommandListHandle commandList);
			void PopulateShaderSceneData(Shader::SceneData& sceneData);

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

			ECS::Entity RootEntity = ECS::InvalidEntity;

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
	}
}