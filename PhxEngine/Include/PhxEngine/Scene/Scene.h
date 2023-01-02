#pragma once

#include <sstream>

#include <PhxEngine/Core/UUID.h>
#include <PhxEngine/Graphics/RHI/PhxRHI.h>
#include <PhxEngine/Scene/Assets.h>
#include <Shaders/ShaderInteropStructures.h>
#include <entt.hpp>

namespace PhxEngine::Scene
{
	class Entity;
	class ISceneWriter;

	class Scene
	{
		friend Entity;
		friend ISceneWriter;
	public:
		static constexpr uint32_t kEnvmapCount = 16;
		static constexpr uint32_t kEnvmapRes = 128;
		static constexpr PhxEngine::RHI::FormatType kEnvmapFormat = PhxEngine::RHI::FormatType::R11G11B10_FLOAT;
		static constexpr PhxEngine::RHI::FormatType kEnvmapDepth = PhxEngine::RHI::FormatType::D16;
		static constexpr uint32_t kEnvmapMIPs = 8;
		static constexpr uint32_t kEnvmapMSAASampleCount = 8;

	public:
		Scene();

		~Scene()
		{
			this->FreeResources();
			this->m_registry.clear();
		};

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

		void ConstructRenderData(
			RHI::CommandListHandle cmd,
			Renderer::ResourceUpload& indexUploader,
			Renderer::ResourceUpload& vertexUploader);

		entt::registry& GetRegistry() { return this->m_registry; }
		const entt::registry& GetRegistry() const { return this->m_registry; }

		void SetBrdfLut(std::shared_ptr<Assets::Texture> texture) { this->m_brdfLut = texture; }
		RHI::DescriptorIndex GetBrdfLutDescriptorIndex();

		PhxEngine::RHI::RTAccelerationStructureHandle GetTlas() const { return this->m_tlas; }

		const Shader::Scene GetShaderData() const { return this->m_shaderData; };

		size_t GetNumInstances() const { return this->m_numInstances; }
		PhxEngine::RHI::BufferHandle GetInstanceBuffer() const { return this->m_instanceGpuBuffer; }
		PhxEngine::RHI::BufferHandle GetInstanceUploadBuffer() const { return this->m_instanceUploadBuffers[RHI::IGraphicsDevice::Ptr->GetFrameIndex()]; }

		size_t GetNumGeometryEntries() const { return this->m_numGeometryEntires; }
		PhxEngine::RHI::BufferHandle GetGeometryBuffer() const { return this->m_geometryGpuBuffer; }
		PhxEngine::RHI::BufferHandle GetGeometryUploadBuffer() const { return this->m_geometryUploadBuffers[RHI::IGraphicsDevice::Ptr->GetFrameIndex()]; }

		size_t GetNumMaterialEntries() const { return this->m_numMaterialEntries; }
		PhxEngine::RHI::BufferHandle GetMaterialBuffer() const { return this->m_materialGpuBuffer; }
		PhxEngine::RHI::BufferHandle GetMaterialUploadBuffer() const { return this->m_materialUploadBuffers[RHI::IGraphicsDevice::Ptr->GetFrameIndex()]; }

		PhxEngine::RHI::RTAccelerationStructureHandle GetTlas() { return this->m_tlas; }
		PhxEngine::RHI::BufferHandle GetTlasUploadBuffer() { return this->m_tlasUploadBuffers[RHI::IGraphicsDevice::Ptr->GetFrameIndex()]; }

		PhxEngine::RHI::TextureHandle GetEnvMapDepthBuffer() const { return this->m_envMapDepthBuffer; }
		PhxEngine::RHI::TextureHandle GetEnvMapArray() const { return this->m_envMapArray; }
		const PhxEngine::RHI::RenderPassHandle GetEnvMapRenderPasses(int index) { return this->m_envMapRenderPasses[index]; }

	public:
		void OnUpdate();

	private:
		void RunMaterialUpdateSystem();
		void RunMeshUpdateSystem();
		void RunProbeUpdateSystem();
		void RunLightUpdateSystem();
		void RunMeshInstanceUpdateSystem();

		void FreeResources();

		void UpdateGpuBufferSizes();

	private:
		Shader::Scene m_shaderData;

		entt::registry m_registry;
		std::shared_ptr<Assets::Texture> m_brdfLut;

		size_t m_numInstances = 0;
		PhxEngine::RHI::BufferHandle m_instanceGpuBuffer;
		std::vector<PhxEngine::RHI::BufferHandle> m_instanceUploadBuffers;

		size_t m_numGeometryEntires = 0;
		PhxEngine::RHI::BufferHandle m_geometryGpuBuffer;
		std::vector<PhxEngine::RHI::BufferHandle> m_geometryUploadBuffers;

		size_t m_numMaterialEntries = 0;
		PhxEngine::RHI::BufferHandle m_materialGpuBuffer;
		std::vector<PhxEngine::RHI::BufferHandle> m_materialUploadBuffers;

		PhxEngine::RHI::RTAccelerationStructureHandle m_tlas;
		std::vector<PhxEngine::RHI::BufferHandle> m_tlasUploadBuffers;


		PhxEngine::RHI::TextureHandle m_envMapDepthBuffer;
		PhxEngine::RHI::TextureHandle m_envMapArray;
		std::array<PhxEngine::RHI::RenderPassHandle, kEnvmapCount> m_envMapRenderPasses;

		entt::entity m_activeSun;
	};

}