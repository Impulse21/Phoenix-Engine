#pragma once

#include <sstream>

#include <PhxEngine/Core/UUID.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Scene/Assets.h>
#include <PhxEngine/Shaders/ShaderInteropStructures.h>
#include <PhxEngine/Shaders/ShaderInteropStructures_NEW.h>
#include <PhxEngine/Renderer/Shadows.h>
#include <PhxEngine/Renderer/DDGI.h>
#include <entt.hpp>
#include <PhxEngine/Core/Memory.h>

namespace PhxEngine
{
	namespace Renderer
	{
		class CommonPasses;
	}
}

namespace PhxEngine::Scene
{
	class Entity;
	class ISceneWriter;

	struct MeshRenderPass
	{
		RHI::BufferHandle IndirectDrawMeshBuffer;
		RHI::BufferHandle IndirectDrawMeshletBuffer;
	};


	class Scene
	{
		friend Entity;
		friend ISceneWriter;
	public:
		static constexpr uint32_t kEnvmapCount = 16;
		static constexpr uint32_t kEnvmapRes = 128;
		static constexpr PhxEngine::RHI::RHIFormat kEnvmapFormat = PhxEngine::RHI::RHIFormat::R11G11B10_FLOAT;
		static constexpr PhxEngine::RHI::RHIFormat kEnvmapDepth = PhxEngine::RHI::RHIFormat::D16;
		static constexpr uint32_t kEnvmapMIPs = 8;
		static constexpr uint32_t kEnvmapMSAASampleCount = 8;

	public:
		Scene();

		~Scene()
		{
			this->FreeResources();
			this->m_registry.clear();
		};

		void Initialize(Core::IAllocator* allocator);

		RHI::ExecutionReceipt BuildRenderData(RHI::IGraphicsDevice* gfxDevice);

		Entity CreateEntity(std::string const& name = std::string());
		Entity CreateEntity(Core::UUID uuid, std::string const& name = std::string());

		void DestroyEntity(Entity entity);

		void AttachToParent(Entity entity, Entity parent, bool childInLocalSpace = false);
		void DetachFromParent(Entity entity);
		void DetachChildren(Entity parent);

		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return this->m_registry.view<Components...>();
		}

		template<typename... Components>
		auto GetAllEntitiesWith() const
		{
			return this->m_registry.view<Components...>();
		}


		entt::registry& GetRegistry() { return this->m_registry; }
		const entt::registry& GetRegistry() const { return this->m_registry; }

		void SetBrdfLut(std::shared_ptr<Assets::Texture> texture) { this->m_brdfLut = texture; }
		RHI::DescriptorIndex GetBrdfLutDescriptorIndex();

		PhxEngine::RHI::RTAccelerationStructureHandle GetTlas() const { return this->m_tlas; }

		const Shader::New::Scene GetShaderData() const { return this->m_shaderData; };

		const Core::AABB& GetBoundingBox() const { return this->m_sceneBounds; }

		Core::IAllocator* GetAllocator() { return this->m_sceneAllocator; }

		RHI::BufferHandle GetGlobalIndexBuffer() const { return this->m_globalIndexBuffer; }

		RHI::BufferHandle GetIndirectDrawEarlyMeshBuffer() { return this->m_indirectDrawEarlyMeshBuffer; }
		RHI::BufferHandle GetIndirectDrawEarlyMeshletBuffer() { return this->m_indirectDrawEarlyMeshletBuffer; }
		RHI::BufferHandle GetCulledInstancesBuffer() { return this->m_culledInstancesBuffer; }
		RHI::BufferHandle GetCulledInstancesCounterBuffer() { return this->m_culledInstancesCounterBuffer; }
		RHI::BufferHandle GetIndirectDrawLateBuffer() { return this->m_indirectDrawLateBuffer; }

		void UpdateBounds();

		PhxEngine::RHI::BufferHandle GetInstanceBuffer() const { return this->m_instanceGpuBuffer; }
		PhxEngine::RHI::BufferHandle GetInstanceUploadBuffer() const { return this->m_instanceUploadBuffers[RHI::IGraphicsDevice::GPtr->GetFrameIndex()]; }

		RHI::BufferHandle GetTlasUploadBuffer() const { return this->m_tlasUploadBuffers[RHI::IGraphicsDevice::GPtr->GetFrameIndex()]; }
		RHI::BufferHandle GetPerlightMeshInstancesCounts() const { return this->m_perlightMeshInstancesCounts; }
		RHI::BufferHandle GetPerlightMeshInstances() const { return this->m_perlightMeshInstances; }
		RHI::BufferHandle GetIndirectDrawShadowMeshBuffer() const { return this->m_indirectDrawShadowMeshBuffer; }
		RHI::BufferHandle GetIndirectDrawShadowMeshletBuffer() const { return this->m_indirectDrawShadowMeshletBuffer; }
		RHI::BufferHandle GetIndirectDrawPerLightCountBuffer() const { return this->m_indirectDrawPerLightCountBuffer; }

		Entity CreateCube(
			RHI::IGraphicsDevice* gfxDevice,
			entt::entity matId = entt::null,
			float size = 1,
			bool rhsCoord = false);

		Entity CreateSphere(
			RHI::IGraphicsDevice* gfxDevice,
			entt::entity matId = entt::null,
			float diameter = 1,
			size_t tessellation = 16,
			bool rhcoords = false);

		Entity CreatePlane(
			RHI::IGraphicsDevice* gfxDevice,
			entt::entity matId = entt::null,
			float width = 1,
			float height = 1,
			bool rhCoords = false);

	public:
		void OnUpdate(std::shared_ptr<Renderer::CommonPasses> commonPasses, bool ddgiEnabled);

		Renderer::DDGI& GetDDGI() { return this->m_ddgi; }

	private:
		void RunMaterialUpdateSystem(std::shared_ptr<Renderer::CommonPasses>& commonPasses);
		void RunMeshUpdateSystem();
		void RunProbeUpdateSystem();
		void RunLightUpdateSystem();
		void RunMeshInstanceUpdateSystem();
		void UpdateRTBuffers();
		void FreeResources();

		void UpdateGpuBufferSizes();

	private:
		void OnConstructOrUpdate(entt::registry& registry, entt::entity entity);

	private:
		void BuildMaterialData(RHI::ICommandList* commandList, RHI::IGraphicsDevice* gfxDevice, std::vector<Renderer::ResourceUpload>& resourcesToFree);
		void BuildMeshData(RHI::ICommandList* commandList, RHI::IGraphicsDevice* gfxDevice);
		void BuildGeometryData(RHI::ICommandList* commandList, RHI::IGraphicsDevice* gfxDevice, std::vector<Renderer::ResourceUpload>& resourcesToFree);
		void BuildIndirectBuffers(RHI::IGraphicsDevice* gfxDevice);
		void BuildSceneData(RHI::ICommandList* commandList, RHI::IGraphicsDevice* gfxDevice);
		void BuildLightBuffers(RHI::IGraphicsDevice* gfxDevice);
		void BuildRTBuffers(RHI::IGraphicsDevice* gfxDevice);

	private:
		Core::IAllocator* m_sceneAllocator;
		Shader::New::Scene m_shaderData;

		Renderer::DDGI m_ddgi;
		entt::registry m_registry;
		std::shared_ptr<Assets::Texture> m_brdfLut;

		RHI::BufferHandle m_globalIndexBuffer;
		RHI::BufferHandle m_globalVertexBuffer;

		RHI::BufferHandle m_globalMeshletBuffer;
		RHI::BufferHandle m_globalUniqueVertexIBBuffer;
		RHI::BufferHandle m_globalMeshletPrimitiveBuffer;
		RHI::BufferHandle m_globalMeshletCullDataBuffer;

		RHI::BufferHandle m_geometryCullDataBuffer;
		RHI::BufferHandle m_geometryGpuBuffer;
		RHI::BufferHandle m_geometryBoundsGpuBuffer;
		RHI::BufferHandle m_materialGpuBuffer;

		bool m_isDirtyMeshInstances = false;
		RHI::BufferHandle m_instanceGpuBuffer;
		std::vector<PhxEngine::RHI::BufferHandle> m_instanceUploadBuffers;

		RHI::BufferHandle m_indirectDrawEarlyBackingBuffer;
		RHI::BufferHandle m_indirectDrawEarlyTransparentBackingBuffer;
		RHI::BufferHandle m_indirectDrawEarlyMeshBuffer;
		RHI::BufferHandle m_indirectDrawEarlyTransparentMeshBuffer;
		RHI::BufferHandle m_indirectDrawEarlyMeshletBuffer;
		RHI::BufferHandle m_indirectDrawEarlyTransparentMeshletBuffer;

		RHI::BufferHandle m_indirectDrawShadowPassMeshBuffer;
		RHI::BufferHandle m_indirectDrawShadowPassMeshletBuffer;
		RHI::BufferHandle m_indirectDrawPerLightCountBuffer;

		RHI::BufferHandle m_perlightMeshInstancesCounts;
		RHI::BufferHandle m_perlightMeshInstances;
		RHI::BufferHandle m_indirectDrawShadowMeshBuffer;
		RHI::BufferHandle m_indirectDrawShadowMeshletBuffer;

		RHI::BufferHandle m_indirectDrawLateBuffer;

		RHI::BufferHandle m_culledInstancesBuffer;
		RHI::BufferHandle m_culledInstancesCounterBuffer;

		PhxEngine::RHI::RTAccelerationStructureHandle m_tlas;
		std::vector<PhxEngine::RHI::BufferHandle> m_tlasUploadBuffers;

		bool m_isDirtyLights = false;
		entt::entity m_activeSun;

		Core::AABB m_sceneBounds;

		// DDGI stuff
	};
}