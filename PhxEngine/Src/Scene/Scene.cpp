#include "phxpch.h"

#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Scene/Entity.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Renderer/CommonPasses.h>

#include <DirectXMath.h>

#include <algorithm>

using namespace PhxEngine;
using namespace PhxEngine::Scene;
using namespace PhxEngine::RHI;
using namespace DirectX;

constexpr uint64_t kVertexBufferAlignment = 16ull;


PhxEngine::Scene::Scene::Scene()
{
	this->m_tlasUploadBuffers.resize(IGraphicsDevice::GPtr->GetMaxInflightFrames());
}

void PhxEngine::Scene::Scene::Initialize(Core::IAllocator* allocator)
{
	this->m_sceneAllocator = allocator;
}

RHI::ExecutionReceipt PhxEngine::Scene::Scene::BuildRenderData(RHI::IGraphicsDevice* gfxDevice)
{
	std::vector<Renderer::ResourceUpload> resourcesToFree;
	RHI::ICommandList* commandList = gfxDevice->BeginCommandRecording();

	this->BuildMaterialData(commandList, gfxDevice, resourcesToFree);
	this->BuildMeshData(commandList, gfxDevice);
	this->BuildGeometryData(commandList, gfxDevice, resourcesToFree);
	this->BuildObjectInstances(commandList, gfxDevice, resourcesToFree);
	this->BuildIndirectBuffers(commandList, gfxDevice);
	this->BuildSceneData(commandList, gfxDevice);

	RHI::ExecutionReceipt retVal = gfxDevice->ExecuteCommandLists({commandList});

	for (auto& resource : resourcesToFree)
	{
		resource.Free();
	}

	return retVal;
}

Entity PhxEngine::Scene::Scene::CreateEntity(std::string const& name)
{
	return this->CreateEntity(Core::UUID(), name);
}

Entity PhxEngine::Scene::Scene::CreateEntity(Core::UUID uuid, std::string const& name)
{
	Entity entity = { this->m_registry.create(), this };
	entity.AddComponent<IDComponent>(uuid);
	entity.AddComponent<TransformComponent>();
	auto& nameComp = entity.AddComponent<NameComponent>();
	nameComp.Name = name.empty() ? "Entity" : name;
	return entity;
}

void PhxEngine::Scene::Scene::DestroyEntity(Entity entity)
{
	this->DetachChildren(entity);
	this->m_registry.destroy(entity);
}

void PhxEngine::Scene::Scene::AttachToParent(Entity entity, Entity parent, bool childInLocalSpace)
{
	assert(entity != parent);
	entity.AttachToParent(parent, childInLocalSpace);
}

void PhxEngine::Scene::Scene::DetachFromParent(Entity entity)
{
	entity.DetachFromParent();
}

void PhxEngine::Scene::Scene::DetachChildren(Entity parent)
{
	parent.DetachChildren();
}

RHI::DescriptorIndex PhxEngine::Scene::Scene::GetBrdfLutDescriptorIndex()
{
	if (!this->m_brdfLut)
	{
		return RHI::cInvalidDescriptorIndex;
	}

	return IGraphicsDevice::GPtr->GetDescriptorIndex(this->m_brdfLut->GetRenderHandle(), RHI::SubresouceType::SRV);
}

void PhxEngine::Scene::Scene::OnUpdate(std::shared_ptr<Renderer::CommonPasses> commonPasses)
{
#ifdef false
	this->m_numMeshlets = 0;

	// Update Geometry Count
	this->UpdateGpuBufferSizes();

	// Clear instance data
	Shader::MeshInstance* pInstanceUploadBufferData = (Shader::MeshInstance*)IGraphicsDevice::GPtr->GetBufferMappedData(this->GetInstanceUploadBuffer());
	if (pInstanceUploadBufferData)
	{
		// Ensure we remove any old data
		std::memset(pInstanceUploadBufferData, 0, IGraphicsDevice::GPtr->GetBufferDesc(this->GetInstanceUploadBuffer()).SizeInBytes);
	}

	this->RunMaterialUpdateSystem(commonPasses);
	this->RunMeshUpdateSystem();
	// TODO: Update Hierarchy
	this->RunProbeUpdateSystem();

	// Update light buffer ? Do this in a per frame setup.
	this->RunLightUpdateSystem();
	this->RunMeshInstanceUpdateSystem();

	// Create Meshlet Buffer from the result of the Mesh sizes;
	if (!this->m_meshletGpuBuffer.IsValid() ||
		IGraphicsDevice::GPtr->GetBufferDesc(this->m_meshletGpuBuffer).SizeInBytes != (this->m_numMeshlets * sizeof(Shader::Meshlet)))
	{
		if (this->m_meshletGpuBuffer.IsValid())
		{
			IGraphicsDevice::GPtr->DeleteBuffer(this->m_meshletGpuBuffer);
		}

		this->m_meshletGpuBuffer = IGraphicsDevice::GPtr->CreateBuffer(
			{
				.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Raw,
				.Binding = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::UnorderedAccess,
				.InitialState = ResourceStates::ShaderResource,
				.StrideInBytes = sizeof(Shader::Meshlet),
				.SizeInBytes = sizeof(Shader::Meshlet) * this->m_numMeshlets,
				.AllowUnorderedAccess = true,
				.CreateBindless = true,
				.DebugName = "Meshlet Buffer"
			});
	}

	// -- Fill Constant Buffer structure for this frame ---
	// this->m_shaderData.NumLights = lightCount;
	this->m_shaderData.MeshInstanceBufferIndex = IGraphicsDevice::GPtr->GetDescriptorIndex(this->m_instanceGpuBuffer, RHI::SubresouceType::SRV);
	this->m_shaderData.GeometryBufferIndex = IGraphicsDevice::GPtr->GetDescriptorIndex(this->m_geometryGpuBuffer, RHI::SubresouceType::SRV);
	this->m_shaderData.MaterialBufferIndex = IGraphicsDevice::GPtr->GetDescriptorIndex(this->m_materialGpuBuffer, RHI::SubresouceType::SRV);
	this->m_shaderData.MeshletBufferIndex = IGraphicsDevice::GPtr->GetDescriptorIndex(this->m_meshletGpuBuffer, RHI::SubresouceType::SRV);
	this->m_shaderData.GlobalIndexBufferIdx = IGraphicsDevice::GPtr->GetDescriptorIndex(this->m_globalIndexBuffer, RHI::SubresouceType::SRV);
	// TODO: Light Buffer
	this->m_shaderData.RT_TlasIndex = IGraphicsDevice::GPtr->GetDescriptorIndex(this->m_tlas);

	// this->m_shaderData.LightEntityIndex = IGraphicsDevice::Ptr->GetDescriptorIndex(this->m_resourceBuffers[RB_LightEntities], RHI::SubresouceType::SRV);
	// this->m_shaderData.MatricesIndex = IGraphicsDevice::Ptr->GetDescriptorIndex(this->m_resourceBuffers[RB_Matrices], RHI::SubresouceType::SRV);
	this->m_shaderData.AtmosphereData = {};
	auto worldEnvView = this->GetRegistry().view<WorldEnvironmentComponent>();
	if (worldEnvView.empty())
	{
		WorldEnvironmentComponent worldComp = {};
		this->m_shaderData.AtmosphereData.ZenithColour = worldComp.ZenithColour;
		this->m_shaderData.AtmosphereData.HorizonColour = worldComp.HorizonColour;
		this->m_shaderData.AtmosphereData.AmbientColour = worldComp.AmbientColour;
		this->m_shaderData.IrradianceMapTexIndex = cInvalidDescriptorIndex;
		this->m_shaderData.PreFilteredEnvMapTexIndex = cInvalidDescriptorIndex;
	}
	else
	{
		auto& worldComp = worldEnvView.get<WorldEnvironmentComponent>(worldEnvView[0]);

		this->m_shaderData.AtmosphereData.ZenithColour = worldComp.ZenithColour;
		this->m_shaderData.AtmosphereData.HorizonColour = worldComp.HorizonColour;
		this->m_shaderData.AtmosphereData.AmbientColour = worldComp.AmbientColour;

		auto image = worldComp.IblTextures[WorldEnvironmentComponent::IrradanceMap];
		if (image && image->GetRenderHandle().IsValid())
		{
			this->m_shaderData.IrradianceMapTexIndex = IGraphicsDevice::GPtr->GetDescriptorIndex(image->GetRenderHandle(), RHI::SubresouceType::SRV);;
		}

		image = worldComp.IblTextures[WorldEnvironmentComponent::PreFilteredEnvMap];
		if (image && image->GetRenderHandle().IsValid())
		{
			this->m_shaderData.PreFilteredEnvMapTexIndex = IGraphicsDevice::GPtr->GetDescriptorIndex(image->GetRenderHandle(), RHI::SubresouceType::SRV);;
		}
	}

	// TODO:
#if false
	auto sun = this->GetRegistry().try_get<LightComponent>(this->m_activeSun);
	if (sun)
	{
		this->m_shaderData.AtmosphereData.SunColour = { sun->Colour.x, sun->Colour.y, sun->Colour.z };
		this->m_shaderData.AtmosphereData.SunDirection = { -sun->Direction.x, -sun->Direction.y, -sun->Direction.z };
	}
	else
	{
		this->m_shaderData.AtmosphereData.SunColour = { };
		this->m_shaderData.AtmosphereData.SunDirection = { };
	}
#else
	this->m_shaderData.AtmosphereData.SunColour = { };
	this->m_shaderData.AtmosphereData.SunDirection = { };
#endif

	this->m_shaderData.EnvMapArray = IGraphicsDevice::GPtr->GetDescriptorIndex(this->m_envMapArray, RHI::SubresouceType::SRV);
	this->m_shaderData.EnvMap_NumMips = kEnvmapMIPs;
#endif

}

void PhxEngine::Scene::Scene::RunMaterialUpdateSystem(std::shared_ptr<Renderer::CommonPasses>& commonPasses)
{
}

void PhxEngine::Scene::Scene::RunMeshUpdateSystem()
{
}

void PhxEngine::Scene::Scene::RunProbeUpdateSystem()
{
#ifdef false
	if (!this->m_envMapArray.IsValid())
	{
		this->m_envMapDepthBuffer = IGraphicsDevice::GPtr->CreateTexture(
			{
				.BindingFlags = BindingFlags::DepthStencil,
				.Dimension = TextureDimension::TextureCube,
				.InitialState = ResourceStates::DepthWrite,
				.Format = kEnvmapDepth,
				.IsBindless = false,
				.Width = kEnvmapRes,
				.Height = kEnvmapRes,
				.ArraySize = 6,
				.OptmizedClearValue = {.DepthStencil = {.Depth = 0.0f }},
				.DebugName = "Env Map Depth"
			});

		// Create EnvMap

		this->m_envMapArray = IGraphicsDevice::GPtr->CreateTexture(
			{
				.BindingFlags = BindingFlags::ShaderResource | BindingFlags::UnorderedAccess | BindingFlags::RenderTarget,
				.Dimension = TextureDimension::TextureCubeArray,
				.InitialState = ResourceStates::RenderTarget,
				.Format = kEnvmapFormat,
				.IsBindless = true,
				.Width = kEnvmapRes,
				.Height = kEnvmapRes,
				.ArraySize = kEnvmapCount * 6,
				.MipLevels = kEnvmapMIPs,
				.OptmizedClearValue = {.Colour = { 0.0f, 0.0f, 0.0f, 1.0f }},
				.DebugName = "Env Map Array"
			});

		// Create sub resource views

		// Cube arrays per mip level:
		for (int i = 0; i < kEnvmapMIPs; i++)
		{
			int subIndex = IGraphicsDevice::GPtr->CreateSubresource(
				this->m_envMapArray,
				RHI::SubresouceType::SRV,
				0,
				kEnvmapCount * 6,
				i,
				1);

			assert(i == subIndex);
			subIndex = IGraphicsDevice::GPtr->CreateSubresource(
				this->m_envMapArray,
				RHI::SubresouceType::UAV,
				0,
				kEnvmapCount * 6,
				i,
				1);
			assert(i == subIndex);
		}

		// individual cubes with mips:
		for (int i = 0; i < kEnvmapCount; i++)
		{
			int subIndex = IGraphicsDevice::GPtr->CreateSubresource(
				this->m_envMapArray,
				RHI::SubresouceType::SRV,
				i * 6,
				6,
				0,
				~0u);
			assert((kEnvmapMIPs + i) == subIndex);
		}

		// individual cubes only mip0:
		for (int i = 0; i < kEnvmapCount; i++)
		{
			int subIndex = IGraphicsDevice::GPtr->CreateSubresource(
				this->m_envMapArray,
				RHI::SubresouceType::SRV,
				i * 6,
				6,
				0,
				1);
			assert((kEnvmapMIPs + kEnvmapCount + i) == subIndex);
		}

		// Create Render Passes
		for (uint32_t i = 0; i < kEnvmapCount; ++i)
		{
			int subIndex = IGraphicsDevice::GPtr->CreateSubresource(
				this->m_envMapArray,
				RHI::SubresouceType::RTV,
				i * 6,
				6,
				0,
				1);

			assert(i == subIndex);

			this->m_envMapRenderPasses[i] = IGraphicsDevice::GPtr->CreateRenderPass(
				{
					.Attachments =
					{
						{
							.LoadOp = RenderPassAttachment::LoadOpType::DontCare,
							.Texture = this->m_envMapArray,
							.Subresource = subIndex,
							.StoreOp = RenderPassAttachment::StoreOpType::Store,
							.InitialLayout = RHI::ResourceStates::RenderTarget,
							.SubpassLayout = RHI::ResourceStates::RenderTarget,
							.FinalLayout = RHI::ResourceStates::RenderTarget
						},
						{
							.Type = RenderPassAttachment::Type::DepthStencil,
							.LoadOp = RenderPassAttachment::LoadOpType::Clear,
							.Texture = this->m_envMapDepthBuffer,
							.StoreOp = RenderPassAttachment::StoreOpType::DontCare,
							.InitialLayout = RHI::ResourceStates::DepthWrite,
							.SubpassLayout = RHI::ResourceStates::DepthWrite,
							.FinalLayout = RHI::ResourceStates::DepthWrite
						},
					}
				});
		}
	}
#endif
}

void PhxEngine::Scene::Scene::RunLightUpdateSystem()
{
#ifdef false
	auto view = this->GetAllEntitiesWith<LightComponent, TransformComponent>();
	for (auto e : view)
	{
		auto [lightComponent, transformComponent] = view.get<LightComponent, TransformComponent>(e);

		XMMATRIX worldMatrix = XMLoadFloat4x4(&transformComponent.WorldMatrix);
		XMVECTOR vScale;
		XMVECTOR vRot;
		XMVECTOR vTranslation;
		XMMatrixDecompose(&vScale, &vRot, &vTranslation, worldMatrix);


		XMStoreFloat3(&lightComponent.Position, vTranslation);
		XMStoreFloat4(&lightComponent.Rotation, vRot);
		XMStoreFloat3(&lightComponent.Scale, vScale);
		XMStoreFloat3(&lightComponent.Direction, XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), worldMatrix)));

		transformComponent.WorldMatrix;

		if (lightComponent.Type == LightComponent::kDirectionalLight)
		{
			this->m_activeSun = e;
		}
	}
#endif
}

void PhxEngine::Scene::Scene::RunMeshInstanceUpdateSystem()
{
}

void PhxEngine::Scene::Scene::FreeResources()
{
	PhxEngine::RHI::IGraphicsDevice::GPtr->DeleteBuffer(this->m_instanceGpuBuffer);
	PhxEngine::RHI::IGraphicsDevice::GPtr->DeleteBuffer(this->m_geometryGpuBuffer);
	PhxEngine::RHI::IGraphicsDevice::GPtr->DeleteBuffer(this->m_materialGpuBuffer);
	PhxEngine::RHI::IGraphicsDevice::GPtr->DeleteBuffer(this->m_globalIndexBuffer);
	PhxEngine::RHI::IGraphicsDevice::GPtr->DeleteBuffer(this->m_globalVertexBuffer);
	PhxEngine::RHI::IGraphicsDevice::GPtr->DeleteBuffer(this->m_globalMeshletBuffer);
	PhxEngine::RHI::IGraphicsDevice::GPtr->DeleteBuffer(this->m_globalMeshletCullDataBuffer);
	PhxEngine::RHI::IGraphicsDevice::GPtr->DeleteBuffer(this->m_globalMeshletPrimitiveBuffer);
	PhxEngine::RHI::IGraphicsDevice::GPtr->DeleteBuffer(this->m_globalUniqueVertexIBBuffer);


	for (int i = 0; i < this->m_tlasUploadBuffers.size(); i++)
	{
		if (this->m_tlasUploadBuffers[i].IsValid())
		{
			IGraphicsDevice::GPtr->DeleteBuffer(this->m_tlasUploadBuffers[i]);
		}
	}

	if (this->m_tlas.IsValid())
	{
		IGraphicsDevice::GPtr->DeleteBuffer(IGraphicsDevice::GPtr->GetRTAccelerationStructureDesc(this->m_tlas).TopLevel.InstanceBuffer);
		IGraphicsDevice::GPtr->DeleteRtAccelerationStructure(this->m_tlas);
	}
}

void PhxEngine::Scene::Scene::UpdateGpuBufferSizes()
{
#ifdef false
	auto meshEntitiesView = this->GetAllEntitiesWith<MeshComponent>();
	this->m_numGeometryEntires = 0;
	for (auto e : meshEntitiesView)
	{
		auto comp = meshEntitiesView.get<MeshComponent>(e);
		this->m_numGeometryEntires += comp.Surfaces.size();
	}

	auto materialEntitiesView = this->GetAllEntitiesWith<MaterialComponent>();
	this->m_numMaterialEntries = materialEntitiesView.size();

	if (!this->m_materialGpuBuffer.IsValid() ||
		IGraphicsDevice::GPtr->GetBufferDesc(this->m_materialGpuBuffer).SizeInBytes != this->m_numMaterialEntries * sizeof(Shader::MaterialData))
	{
		RHI::BufferDesc desc = {};
		desc.DebugName = "Material Data";
		desc.Binding = RHI::BindingFlags::ShaderResource;
		desc.InitialState = ResourceStates::ShaderResource;
		desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Raw;
		desc.CreateBindless = true;
		desc.StrideInBytes = sizeof(Shader::MaterialData);
		desc.SizeInBytes = sizeof(Shader::MaterialData) * this->m_numMaterialEntries;

		if (this->m_materialGpuBuffer.IsValid())
		{
			IGraphicsDevice::GPtr->DeleteBuffer(this->m_materialGpuBuffer);
		}
		this->m_materialGpuBuffer = IGraphicsDevice::GPtr->CreateBuffer(desc);

		desc.CreateBindless = false;
		desc.DebugName = "Material Upload Data";
		desc.Usage = RHI::Usage::Upload;
		desc.Binding = RHI::BindingFlags::None;
		desc.MiscFlags = RHI::BufferMiscFlags::None;
		desc.InitialState = ResourceStates::CopySource;
		for (int i = 0; i < this->m_materialUploadBuffers.size(); i++)
		{
			if (this->m_materialUploadBuffers[i].IsValid())
			{
				IGraphicsDevice::GPtr->DeleteBuffer(this->m_materialUploadBuffers[i]);
			}
			this->m_materialUploadBuffers[i] = IGraphicsDevice::GPtr->CreateBuffer(desc);
		}
	}

	if (!this->m_geometryGpuBuffer.IsValid() ||
		IGraphicsDevice::GPtr->GetBufferDesc(this->m_geometryGpuBuffer).SizeInBytes != (this->m_numGeometryEntires * sizeof(Shader::Geometry)))
	{
		RHI::BufferDesc desc = {};
		desc.DebugName = "Geometry Data";
		desc.Binding = RHI::BindingFlags::ShaderResource;
		desc.InitialState = ResourceStates::ShaderResource;
		desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Raw;
		desc.CreateBindless = true;
		desc.StrideInBytes = sizeof(Shader::Geometry);
		desc.SizeInBytes = sizeof(Shader::Geometry) * this->m_numGeometryEntires;

		if (this->m_geometryGpuBuffer.IsValid())
		{
			IGraphicsDevice::GPtr->DeleteBuffer(this->m_geometryGpuBuffer);
		}
		this->m_geometryGpuBuffer = IGraphicsDevice::GPtr->CreateBuffer(desc);

		desc.DebugName = "Geometry Upload";
		desc.Usage = RHI::Usage::Upload;
		desc.Binding = RHI::BindingFlags::None;
		desc.MiscFlags = RHI::BufferMiscFlags::None;
		desc.InitialState = ResourceStates::CopySource;
		desc.CreateBindless = false;
		for (int i = 0; i < this->m_geometryUploadBuffers.size(); i++)
		{
			if (this->m_geometryUploadBuffers[i].IsValid())
			{
				IGraphicsDevice::GPtr->DeleteBuffer(this->m_geometryUploadBuffers[i]);
			}
			this->m_geometryUploadBuffers[i] = IGraphicsDevice::GPtr->CreateBuffer(desc);
		}
	}

	auto meshInstanceView = this->GetAllEntitiesWith<MeshInstanceComponent>();
	this->m_numInstances = meshInstanceView.size();
	if (!this->m_instanceGpuBuffer.IsValid() ||
		IGraphicsDevice::GPtr->GetBufferDesc(this->m_instanceGpuBuffer).SizeInBytes != (this->m_numInstances * sizeof(Shader::MeshInstance)))
	{
		RHI::BufferDesc desc = {};
		desc.DebugName = "Instance Data";
		desc.Binding = RHI::BindingFlags::ShaderResource;
		desc.InitialState = ResourceStates::ShaderResource;
		desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Raw;
		desc.CreateBindless = true;
		desc.StrideInBytes = sizeof(Shader::MeshInstance);
		desc.SizeInBytes = sizeof(Shader::MeshInstance) * this->m_numInstances;

		if (this->m_instanceGpuBuffer.IsValid())
		{
			IGraphicsDevice::GPtr->DeleteBuffer(this->m_instanceGpuBuffer);
		}
		this->m_instanceGpuBuffer = IGraphicsDevice::GPtr->CreateBuffer(desc);

		desc.DebugName = "Geometry Upload";
		desc.Usage = RHI::Usage::Upload;
		desc.Binding = RHI::BindingFlags::None;
		desc.MiscFlags = RHI::BufferMiscFlags::None;
		desc.InitialState = ResourceStates::CopySource;
		desc.CreateBindless = false;
		for (int i = 0; i < this->m_instanceUploadBuffers.size(); i++)
		{
			if (this->m_instanceUploadBuffers[i].IsValid())
			{
				IGraphicsDevice::GPtr->DeleteBuffer(this->m_instanceUploadBuffers[i]);
			}
			this->m_instanceUploadBuffers[i] = IGraphicsDevice::GPtr->CreateBuffer(desc);
		}
	}

	// Set up TLAS Data
	if (IGraphicsDevice::GPtr->CheckCapability(DeviceCapability::RayTracing))
	{
		BufferDesc desc;
		desc.StrideInBytes = IGraphicsDevice::GPtr->GetRTTopLevelAccelerationStructureInstanceSize();
		desc.SizeInBytes = desc.StrideInBytes * this->m_numInstances * 2; // *2 to grow fast
		desc.Usage = Usage::Upload;
		desc.DebugName = "TLAS Upload Buffer";

		if (!this->m_tlasUploadBuffers.front().IsValid() || IGraphicsDevice::GPtr->GetBufferDesc(this->m_tlasUploadBuffers.front()).SizeInBytes < desc.SizeInBytes)
		{
			for (int i = 0; i < this->m_tlasUploadBuffers.size(); i++)
			{
				if (this->m_tlasUploadBuffers[i].IsValid())
				{
					IGraphicsDevice::GPtr->DeleteBuffer(this->m_tlasUploadBuffers[i]);
				}
				this->m_tlasUploadBuffers[i] = IGraphicsDevice::GPtr->CreateBuffer(desc);
			}
		}

		BufferHandle currentTlasUploadBuffer = this->m_tlasUploadBuffers[IGraphicsDevice::GPtr->GetFrameIndex()];
		void* pTlasUploadBufferData = IGraphicsDevice::GPtr->GetBufferMappedData(currentTlasUploadBuffer);

		if (pTlasUploadBufferData)
		{
			// Ensure we remove any old data
			std::memset(pTlasUploadBufferData, 0, IGraphicsDevice::GPtr->GetBufferDesc(currentTlasUploadBuffer).SizeInBytes);

			int instanceId = 0;
			auto viewMeshTranslation = this->GetAllEntitiesWith<MeshInstanceComponent, TransformComponent>();
			for (auto e : viewMeshTranslation)
			{
				auto [meshInstanceComponent, transformComponent] = viewMeshTranslation.get<MeshInstanceComponent, TransformComponent>(e);

				RTAccelerationStructureDesc::TopLevelDesc::Instance instance = {};
				for (int i = 0; i < ARRAYSIZE(instance.Transform); ++i)
				{
					for (int j = 0; j < ARRAYSIZE(instance.Transform[i]); ++j)
					{
						instance.Transform[i][j] = transformComponent.WorldMatrix.m[j][i];
					}
				}

				auto& meshComponent = this->m_registry.get<MeshComponent>(meshInstanceComponent.Mesh);
				instance.InstanceId = instanceId++;
				instance.InstanceMask = 0xff;
				instance.BottomLevel = meshComponent.Blas;
				instance.InstanceContributionToHitGroupIndex = 0;
				instance.Flags = 0;

				// TODO: Disable cull for Two-sided materials.
				/*
				if (meshComponent)
				{
					instance.flags |= RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_TRIANGLE_CULL_DISABLE;
				}
				if (XMVectorGetX(XMMatrixDeterminant(W)) > 0)
				{
					// There is a mismatch between object space winding and BLAS winding:
					//	https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_raytracing_instance_flags
					// instance.flags |= RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;
				}
				*/

				void* dest = (void*)((size_t)pTlasUploadBufferData + (size_t)instance.InstanceId * IGraphicsDevice::GPtr->GetRTTopLevelAccelerationStructureInstanceSize());
				IGraphicsDevice::GPtr->WriteRTTopLevelAccelerationStructureInstance(instance, dest);
			}
		}


		if (!this->m_tlas.IsValid() || IGraphicsDevice::GPtr->GetRTAccelerationStructureDesc(this->m_tlas).TopLevel.Count < this->m_numInstances)
		{
			RHI::RTAccelerationStructureDesc desc;
			desc.Flags = RHI::RTAccelerationStructureDesc::kPreferFastBuild;
			desc.Type = RHI::RTAccelerationStructureDesc::Type::TopLevel;
			desc.TopLevel.Count = (uint32_t)this->m_numInstances * 2; // *2 to grow fast

			RHI::BufferDesc bufferDesc = {};
			bufferDesc.MiscFlags = BufferMiscFlags::Structured; // TODO: RayTracing
			bufferDesc.StrideInBytes = IGraphicsDevice::GPtr->GetRTTopLevelAccelerationStructureInstanceSize();
			bufferDesc.SizeInBytes = bufferDesc.StrideInBytes * desc.TopLevel.Count;
			bufferDesc.DebugName = "TLAS::InstanceBuffer";

			if (this->m_tlas.IsValid())
			{
				IGraphicsDevice::GPtr->DeleteBuffer(IGraphicsDevice::GPtr->GetRTAccelerationStructureDesc(this->m_tlas).TopLevel.InstanceBuffer);
			}

			desc.TopLevel.InstanceBuffer = IGraphicsDevice::GPtr->CreateBuffer(bufferDesc);

			IGraphicsDevice::GPtr->DeleteRtAccelerationStructure(this->m_tlas);
			this->m_tlas = IGraphicsDevice::GPtr->CreateRTAccelerationStructure(desc);
		}
	}
#endif
}


void PhxEngine::Scene::Scene::BuildGeometryData(RHI::ICommandList* commandList, RHI::IGraphicsDevice* gfxDevice, std::vector<Renderer::ResourceUpload>& resourcesToFree)
{
	assert(false);
	auto meshView = this->GetAllEntitiesWith<MeshComponent>();
	const size_t geometryBufferSize = sizeof(Shader::New::Geometry) * meshView.size();

	RHI::BufferDesc desc = {};
	desc.DebugName = "Geometry Data";
	desc.Binding = RHI::BindingFlags::ShaderResource;
	desc.InitialState = ResourceStates::ShaderResource;
	desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Structured;
	desc.CreateBindless = true;
	desc.StrideInBytes = sizeof(Shader::New::Geometry);
	desc.SizeInBytes = geometryBufferSize;

	if (this->m_geometryGpuBuffer.IsValid())
	{
		IGraphicsDevice::GPtr->DeleteBuffer(this->m_geometryGpuBuffer);
	}
	this->m_geometryGpuBuffer = IGraphicsDevice::GPtr->CreateBuffer(desc);

	Renderer::ResourceUpload& geometryUploadBuffer = resourcesToFree.emplace_back(Renderer::CreateResourceUpload(geometryBufferSize));
	Shader::New::Geometry* geometryBufferMappedData = (Shader::New::Geometry*)geometryUploadBuffer.Data;

	uint32_t currGeoIndex = 0;
	for (auto entity : meshView)
	{
		auto& mesh = meshView.get<MeshComponent>(entity);
		mesh.GlobalIndexOffsetGeometryBuffer = currGeoIndex++;

		Shader::New::Geometry* geometryShaderData = geometryBufferMappedData + mesh.GlobalIndexOffsetGeometryBuffer;
		auto& material = this->GetRegistry().get<MaterialComponent>(mesh.Material);
		geometryShaderData->MaterialIndex = material.GlobalBufferIndex;

		geometryShaderData->NumIndices = mesh.TotalIndices;
		geometryShaderData->NumVertices = mesh.TotalVertices;
		geometryShaderData->IndexByteOffset = mesh.GlobalByteOffsetIndexBuffer;

		geometryShaderData->VertexBufferIndex = IGraphicsDevice::GPtr->GetDescriptorIndex(this->m_globalIndexBuffer, RHI::SubresouceType::SRV);
		geometryShaderData->PositionOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Position).ByteOffset;
		geometryShaderData->TexCoordOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::TexCoord).ByteOffset;
		geometryShaderData->NormalOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Normal).ByteOffset;
		geometryShaderData->TangentOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Tangent).ByteOffset;
	}
}

void PhxEngine::Scene::Scene::BuildObjectInstances(RHI::ICommandList* commandList, RHI::IGraphicsDevice* gfxDevice, std::vector<Renderer::ResourceUpload>& resourcesToFree)
{
	auto instanceView = this->GetAllEntitiesWith<MeshInstanceComponent>();
	const size_t instanceBufferSizeInBytes = sizeof(Shader::New::ObjectInstance) * instanceView.size();
	RHI::BufferDesc desc = {};
	desc.DebugName = "Instance Data";
	desc.Binding = RHI::BindingFlags::ShaderResource;
	desc.InitialState = ResourceStates::ShaderResource;
	desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Structured;
	desc.CreateBindless = true;
	desc.StrideInBytes = sizeof(Shader::New::ObjectInstance);
	desc.SizeInBytes = sizeof(Shader::New::ObjectInstance) * instanceView.size();

	if (this->m_instanceGpuBuffer.IsValid())
	{
		IGraphicsDevice::GPtr->DeleteBuffer(this->m_instanceGpuBuffer);
	}
	this->m_instanceGpuBuffer = IGraphicsDevice::GPtr->CreateBuffer(desc);

	Renderer::ResourceUpload& uploadBuffer = resourcesToFree.emplace_back(Renderer::CreateResourceUpload(instanceBufferSizeInBytes));
	Shader::New::ObjectInstance* instancePtr = (Shader::New::ObjectInstance*)uploadBuffer.Data;


	auto instanceTransformView = this->GetAllEntitiesWith<MeshInstanceComponent, TransformComponent>();

	uint32_t currInstanceIndex = 0;
	for (auto e : instanceTransformView)
	{
		auto [meshInstanceComponent, transformComponent] = instanceTransformView.get<MeshInstanceComponent, TransformComponent>(e);
		Shader::New::ObjectInstance* shaderMeshInstance = instancePtr + currInstanceIndex;

		auto& mesh = this->GetRegistry().get<MeshComponent>(meshInstanceComponent.Mesh);
		shaderMeshInstance->GeometryIndex = mesh.GlobalIndexOffsetGeometryBuffer;
		shaderMeshInstance->WorldMatrix = transformComponent.WorldMatrix;
		shaderMeshInstance->MeshletOffset = mesh.GlobalIndexOffsetMeshletBuffer;
		meshInstanceComponent.GlobalBufferIndex = currInstanceIndex++;
	}

	auto aabbView = this->GetAllEntitiesWith<MeshInstanceComponent, AABBComponent>();
	for (auto e : aabbView)
	{
		auto [meshInstanceComponent, aabbComponent] = aabbView.get<MeshInstanceComponent, AABBComponent>(e);
		auto& mesh = this->GetRegistry().get<MeshComponent>(meshInstanceComponent.Mesh);

		aabbComponent.BoundingData = mesh.Aabb.Transform(DirectX::XMLoadFloat4x4(&meshInstanceComponent.WorldMatrix));
		this->m_sceneBounds = Core::AABB::Merge(this->m_sceneBounds, aabbComponent.BoundingData);
	}
}

void PhxEngine::Scene::Scene::BuildIndirectBuffers(RHI::ICommandList* commandList, RHI::IGraphicsDevice* gfxDevice)
{
	auto view = this->GetAllEntitiesWith<MeshInstanceComponent>();

	if (this->m_indirectDrawEarlyBuffer.IsValid())
	{
		gfxDevice->DeleteBuffer(this->m_indirectDrawEarlyBuffer);
	}

	this->m_indirectDrawEarlyBuffer = gfxDevice->CreateBuffer({
			   .MiscFlags = BufferMiscFlags::Structured | BufferMiscFlags::HasCounter,
			   .Binding = BindingFlags::UnorderedAccess,
			   .InitialState = ResourceStates::IndirectArgument,
			   .StrideInBytes = sizeof(Shader::New::MeshDrawCommand),
			   .SizeInBytes = sizeof(Shader::New::MeshDrawCommand) * view.size() + sizeof(uint32_t),
			   .AllowUnorderedAccess = true,
			   .UavCounterOffsetInBytes = sizeof(Shader::New::MeshDrawCommand) * view.size() });

	if (this->m_indirectDrawCulledBuffer.IsValid())
	{
		gfxDevice->DeleteBuffer(this->m_indirectDrawCulledBuffer);
	}
	this->m_indirectDrawCulledBuffer = gfxDevice->CreateBuffer({
			   .MiscFlags = BufferMiscFlags::Structured | BufferMiscFlags::HasCounter,
			   .Binding = BindingFlags::UnorderedAccess,
			   .InitialState = ResourceStates::IndirectArgument,
			   .StrideInBytes = sizeof(Shader::New::MeshDrawCommand),
			   .SizeInBytes = sizeof(Shader::New::MeshDrawCommand) * view.size() + sizeof(uint32_t),
			   .AllowUnorderedAccess = true,
			   .UavCounterOffsetInBytes = sizeof(Shader::New::MeshDrawCommand) * view.size() });

	if (this->m_indirectDrawLateBuffer.IsValid())
	{
		gfxDevice->DeleteBuffer(this->m_indirectDrawLateBuffer);
	}
	this->m_indirectDrawLateBuffer = gfxDevice->CreateBuffer({
			   .MiscFlags = BufferMiscFlags::Structured | BufferMiscFlags::HasCounter,
			   .Binding = BindingFlags::UnorderedAccess,
			   .InitialState = ResourceStates::IndirectArgument,
			   .StrideInBytes = sizeof(Shader::New::MeshDrawCommand),
			   .SizeInBytes = sizeof(Shader::New::MeshDrawCommand) * view.size() + sizeof(uint32_t),
			   .AllowUnorderedAccess = true,
			   .UavCounterOffsetInBytes = sizeof(Shader::New::MeshDrawCommand) * view.size() });
}

void PhxEngine::Scene::Scene::BuildSceneData(RHI::ICommandList* commandList, RHI::IGraphicsDevice* gfxDevice)
{
	// TODO: Set Scene Data strcut
	assert(false);
}

void PhxEngine::Scene::Scene::BuildMaterialData(RHI::ICommandList* commandList, RHI::IGraphicsDevice* gfxDevice, std::vector<Renderer::ResourceUpload>& resourcesToFree)
{
	auto mtlView = this->GetAllEntitiesWith<MaterialComponent>();
	const size_t mtlBufferSize = sizeof(Shader::New::Material) * mtlView.size();

	RHI::BufferDesc desc = {};
	desc.DebugName = "Material Data";
	desc.Binding = RHI::BindingFlags::ShaderResource;
	desc.InitialState = ResourceStates::ShaderResource;
	desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Structured;
	desc.CreateBindless = true;
	desc.StrideInBytes = sizeof(Shader::New::Material);
	desc.SizeInBytes = mtlBufferSize;

	if (this->m_materialGpuBuffer.IsValid())
	{
		IGraphicsDevice::GPtr->DeleteBuffer(this->m_materialGpuBuffer);
	}
	this->m_materialGpuBuffer = IGraphicsDevice::GPtr->CreateBuffer(desc);

	Renderer::ResourceUpload& uploadBuffer = resourcesToFree.emplace_back(Renderer::CreateResourceUpload(mtlBufferSize));
	Shader::New::Material* uploadData = (Shader::New::Material*)uploadBuffer.Data;

	auto view = this->GetAllEntitiesWith<MaterialComponent>();
	uint32_t currMat = 0;
	for (auto entity : view)
	{
		auto& mat = view.get<MaterialComponent>(entity);

		Shader::New::Material* shaderData = uploadData + currMat;

		shaderData->AlbedoColour = { mat.BaseColour.x, mat.BaseColour.y, mat.BaseColour.z };
		shaderData->EmissiveColourPacked = Core::Math::PackColour(mat.Emissive);
		shaderData->AO = mat.Ao;
		shaderData->Metalness = mat.Metalness;
		shaderData->Roughness = mat.Roughness;

		shaderData->AlbedoTexture = RHI::cInvalidDescriptorIndex;
		if (mat.BaseColourTexture && mat.BaseColourTexture->GetRenderHandle().IsValid())
		{
			shaderData->AlbedoTexture = RHI::IGraphicsDevice::GPtr->GetDescriptorIndex(mat.BaseColourTexture->GetRenderHandle(), RHI::SubresouceType::SRV);
		}

		shaderData->AOTexture = RHI::cInvalidDescriptorIndex;
		if (mat.AoTexture && mat.AoTexture->GetRenderHandle().IsValid())
		{
			shaderData->AOTexture = RHI::IGraphicsDevice::GPtr->GetDescriptorIndex(mat.AoTexture->GetRenderHandle(), RHI::SubresouceType::SRV);
		}

		shaderData->MaterialTexture = RHI::cInvalidDescriptorIndex;
		if (mat.MetalRoughnessTexture && mat.MetalRoughnessTexture->GetRenderHandle().IsValid())
		{
			shaderData->MaterialTexture = RHI::IGraphicsDevice::GPtr->GetDescriptorIndex(mat.MetalRoughnessTexture->GetRenderHandle(), RHI::SubresouceType::SRV);
			assert(shaderData->MaterialTexture != RHI::cInvalidDescriptorIndex);

			// shaderData->MetalnessTexture = mat.MetalRoughnessTexture->GetDescriptorIndex();
			// shaderData->RoughnessTexture = mat.MetalRoughnessTexture->GetDescriptorIndex();
		}

		shaderData->NormalTexture = RHI::cInvalidDescriptorIndex;
		if (mat.NormalMapTexture && mat.NormalMapTexture->GetRenderHandle().IsValid())
		{
			shaderData->NormalTexture = RHI::IGraphicsDevice::GPtr->GetDescriptorIndex(mat.NormalMapTexture->GetRenderHandle(), RHI::SubresouceType::SRV);
		}

		mat.GlobalBufferIndex = currMat++;
	}
}

void PhxEngine::Scene::Scene::BuildMeshData(RHI::ICommandList* commandList, RHI::IGraphicsDevice* gfxDevice)
{
	// Construct Mesh Render Data
	auto meshView = this->GetAllEntitiesWith<MeshComponent>();
	size_t totalVertexCount = 0;
	size_t totalIndexCount = 0;
	size_t totalMeshletCount = 0;
	size_t totalUniqueVertexIBCount = 0;
	size_t totalMeshletPrimitiveCount = 0;
	for (auto e : meshView)
	{
		auto& mesh = meshView.get<MeshComponent>(e);

		mesh.GlobalByteOffsetIndexBuffer = totalIndexCount;
		mesh.GlobalByteOffsetVertexBuffer = totalVertexCount;

		mesh.GlobalIndexOffsetMeshletBuffer = totalMeshletCount;
		mesh.GlobalIndexOffsetUnqiueVertexIBBuffer = totalUniqueVertexIBCount;
		mesh.GlobalIndexOffsetMeshletPrimitiveBuffer = totalMeshletPrimitiveCount;
		mesh.GlobalIndexOffsetMeshletCullDataBuffer = totalMeshletCount;

		totalIndexCount += gfxDevice->GetBufferDesc(mesh.IndexBuffer).SizeInBytes;
		totalVertexCount += gfxDevice->GetBufferDesc(mesh.VertexBuffer).SizeInBytes;

		totalMeshletCount += mesh.Meshlets.size();
		totalUniqueVertexIBCount += mesh.UniqueVertexIB.size();
		totalMeshletPrimitiveCount += mesh.MeshletTriangles.size();
	}

	this->m_globalIndexBuffer = gfxDevice->CreateIndexBuffer({
			.StrideInBytes = sizeof(uint32_t),
			.SizeInBytes = totalIndexCount,
			.DebugName = "Scene Index Buffer" });

	this->m_globalVertexBuffer = gfxDevice->CreateIndexBuffer({
			.MiscFlags = RHI::BufferMiscFlags::Raw | RHI::BufferMiscFlags::Bindless,
			.Binding = RHI::BindingFlags::VertexBuffer | RHI::BindingFlags::ShaderResource,
			.StrideInBytes = sizeof(float),
			.SizeInBytes = totalVertexCount,
			.DebugName = "Scene Vertex Buffer" });

	this->m_globalMeshletBuffer = gfxDevice->CreateBuffer({
			.MiscFlags = RHI::BufferMiscFlags::Structured | RHI::BufferMiscFlags::Bindless,
			.Binding = RHI::BindingFlags::ShaderResource,
			.StrideInBytes = sizeof(DirectX::Meshlet),
			.SizeInBytes = totalMeshletCount * sizeof(DirectX::Meshlet),
			.DebugName = "Scene Meshlet Buffer" });
	const size_t meshletBufferStride = gfxDevice->GetBufferDesc(this->m_globalMeshletBuffer).StrideInBytes;

	this->m_globalUniqueVertexIBBuffer = gfxDevice->CreateBuffer({
			.MiscFlags = RHI::BufferMiscFlags::Raw | RHI::BufferMiscFlags::Bindless,
			.Binding = RHI::BindingFlags::ShaderResource,
			.SizeInBytes = totalUniqueVertexIBCount * sizeof(uint8_t),
			.DebugName = "Scene Meshlet Unique Vertex IB" });
	const size_t uniqueVertexIBStride= gfxDevice->GetBufferDesc(this->m_globalUniqueVertexIBBuffer).StrideInBytes;

	this->m_globalMeshletPrimitiveBuffer = gfxDevice->CreateBuffer({
			.MiscFlags = RHI::BufferMiscFlags::Raw | RHI::BufferMiscFlags::Bindless,
			.Binding = RHI::BindingFlags::ShaderResource,
			.SizeInBytes = totalMeshletPrimitiveCount * sizeof(DirectX::MeshletTriangle),
			.DebugName = "Scene Meshlet Primitives" });
	const size_t meshletPrimtiveStide = gfxDevice->GetBufferDesc(this->m_globalMeshletPrimitiveBuffer).StrideInBytes;

	this->m_globalMeshletCullDataBuffer = gfxDevice->CreateBuffer({
			.MiscFlags = RHI::BufferMiscFlags::Structured | RHI::BufferMiscFlags::Bindless,
			.Binding = RHI::BindingFlags::ShaderResource,
			.StrideInBytes = sizeof(DirectX::CullData),
			.SizeInBytes = totalMeshletCount * sizeof(DirectX::CullData),
			.DebugName = "Scene Meshlet Cull Data" });
	const size_t meshletCullDataStride = gfxDevice->GetBufferDesc(this->m_globalMeshletCullDataBuffer).StrideInBytes;

	// -- Upload Geometry Data ---
	{
		RHI::GpuBarrier preCopyBarriers[] =
		{
			RHI::GpuBarrier::CreateBuffer(this->m_globalIndexBuffer, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest),
			RHI::GpuBarrier::CreateBuffer(this->m_globalVertexBuffer, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest),
			RHI::GpuBarrier::CreateBuffer(this->m_globalMeshletBuffer, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest),
			RHI::GpuBarrier::CreateBuffer(this->m_globalUniqueVertexIBBuffer, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest),
			RHI::GpuBarrier::CreateBuffer(this->m_globalMeshletPrimitiveBuffer, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest),
			RHI::GpuBarrier::CreateBuffer(this->m_globalMeshletCullDataBuffer, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest),
		};

		commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));
	}

	for (auto e : meshView)
	{
		auto& mesh = meshView.get<MeshComponent>(e);
		RHI::GpuBarrier preCopyBarriers[] =
		{
			RHI::GpuBarrier::CreateBuffer(mesh.VertexBuffer, RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopySource),
			RHI::GpuBarrier::CreateBuffer(mesh.IndexBuffer, RHI::ResourceStates::IndexGpuBuffer | RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopySource),
		};

		commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));

		commandList->CopyBuffer(
			this->m_globalIndexBuffer,
			mesh.GlobalByteOffsetIndexBuffer,
			mesh.IndexBuffer,
			0,
			gfxDevice->GetBufferDesc(mesh.IndexBuffer).SizeInBytes);

		commandList->CopyBuffer(
			this->m_globalVertexBuffer,
			mesh.GlobalByteOffsetVertexBuffer,
			mesh.VertexBuffer,
			0,
			gfxDevice->GetBufferDesc(mesh.VertexBuffer).SizeInBytes);

		commandList->CopyBuffer(
			this->m_globalMeshletBuffer,
			mesh.GlobalIndexOffsetMeshletBuffer * meshletBufferStride,
			mesh.MeshletBuffer,
			0,
			gfxDevice->GetBufferDesc(mesh.MeshletBuffer).SizeInBytes);

		commandList->CopyBuffer(
			this->m_globalUniqueVertexIBBuffer,
			mesh.GlobalIndexOffsetUnqiueVertexIBBuffer * uniqueVertexIBStride,
			mesh.UniqueVertexIBBuffer,
			0,
			gfxDevice->GetBufferDesc(mesh.UniqueVertexIBBuffer).SizeInBytes);

		commandList->CopyBuffer(
			this->m_globalMeshletPrimitiveBuffer,
			mesh.GlobalIndexOffsetMeshletPrimitiveBuffer * meshletPrimtiveStide,
			mesh.MeshletPrimitivesBuffer,
			0,
			gfxDevice->GetBufferDesc(mesh.MeshletPrimitivesBuffer).SizeInBytes);

		commandList->CopyBuffer(
			this->m_globalMeshletCullDataBuffer,
			mesh.GlobalIndexOffsetMeshletCullDataBuffer * meshletCullDataStride,
			mesh.MeshletCullDataBuffer,
			0,
			gfxDevice->GetBufferDesc(mesh.MeshletCullDataBuffer).SizeInBytes);

		// Upload data
		RHI::GpuBarrier postCopyBarriers[] =
		{
			RHI::GpuBarrier::CreateBuffer(mesh.VertexBuffer, RHI::ResourceStates::CopySource, RHI::ResourceStates::ShaderResource),
			RHI::GpuBarrier::CreateBuffer(mesh.IndexBuffer, RHI::ResourceStates::CopySource, RHI::ResourceStates::IndexGpuBuffer | RHI::ResourceStates::ShaderResource),
		};

		commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postCopyBarriers, _countof(postCopyBarriers)));
	}

	{
		RHI::GpuBarrier postCopyBarriers[] =
		{
			RHI::GpuBarrier::CreateBuffer(this->m_globalIndexBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::IndexGpuBuffer | RHI::ResourceStates::ShaderResource),
			RHI::GpuBarrier::CreateBuffer(this->m_globalVertexBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
			RHI::GpuBarrier::CreateBuffer(this->m_globalMeshletBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
			RHI::GpuBarrier::CreateBuffer(this->m_globalUniqueVertexIBBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
			RHI::GpuBarrier::CreateBuffer(this->m_globalMeshletPrimitiveBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
			RHI::GpuBarrier::CreateBuffer(this->m_globalMeshletCullDataBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
		};

		commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postCopyBarriers, _countof(postCopyBarriers)));
	}
}
