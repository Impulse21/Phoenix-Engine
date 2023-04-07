#include "phxpch.h"

#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Core/Primitives.h>

#include <PhxEngine/Scene/Entity.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Renderer/CommonPasses.h>


#include <DirectXMath.h>

#include <algorithm>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Scene;
using namespace PhxEngine::RHI;
using namespace DirectX;

constexpr uint64_t kVertexBufferAlignment = 16ull;


PhxEngine::Scene::Scene::Scene()
{
	this->m_tlasUploadBuffers.resize(IGraphicsDevice::GPtr->GetMaxInflightFrames());
	this->m_instanceUploadBuffers.resize(IGraphicsDevice::GPtr->GetMaxInflightFrames());
	this->m_lightUploadBuffers.resize(IGraphicsDevice::GPtr->GetMaxInflightFrames());
}

void PhxEngine::Scene::Scene::Initialize(Core::IAllocator* allocator)
{
	this->m_sceneAllocator = allocator;
	/*
	this->GetRegistry().on_construct<MeshInstanceComponent>().connect<OnConstructOrUpdate>();
	this->GetRegistry().on_update<MeshInstanceComponent>().connect<OnConstructOrUpdate>();

	this->GetRegistry().on_construct<LightComponent>().connect<OnConstructOrUpdate>();
	this->GetRegistry().on_update<LightComponent>().connect<OnConstructOrUpdate>();
	*/
}

RHI::ExecutionReceipt PhxEngine::Scene::Scene::BuildRenderData(RHI::IGraphicsDevice* gfxDevice)
{
	std::vector<Renderer::ResourceUpload> resourcesToFree;
	RHI::ICommandList* commandList = gfxDevice->BeginCommandRecording();

	this->BuildMaterialData(commandList, gfxDevice, resourcesToFree);
	this->BuildMeshData(commandList, gfxDevice);
	this->BuildGeometryData(commandList, gfxDevice, resourcesToFree);
	this->BuildIndirectBuffers(gfxDevice);

	commandList->Close();
	RHI::ExecutionReceipt retVal = gfxDevice->ExecuteCommandLists({commandList});

	for (auto& resource : resourcesToFree)
	{
		resource.Free();
	}

	this->BuildSceneData(commandList, gfxDevice);
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

void PhxEngine::Scene::Scene::UpdateBounds()
{
	auto aabbView = this->GetAllEntitiesWith<MeshInstanceComponent, AABBComponent, TransformComponent>();
	for (auto e : aabbView)
	{
		auto [meshInstanceComponent, aabbComponent, transformComponent] = aabbView.get<MeshInstanceComponent, AABBComponent, TransformComponent>(e);
		auto& mesh = this->GetRegistry().get<MeshComponent>(meshInstanceComponent.Mesh);

		aabbComponent.BoundingData = mesh.Aabb.Transform(DirectX::XMLoadFloat4x4(&transformComponent.WorldMatrix));
		this->m_sceneBounds = Core::AABB::Merge(this->m_sceneBounds, aabbComponent.BoundingData);
	}
}

void PhxEngine::Scene::Scene::OnUpdate(std::shared_ptr<Renderer::CommonPasses> commonPasses)
{
	// Update Light Data
	this->RunMeshInstanceUpdateSystem();
	this->RunLightUpdateSystem();

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
	uint32_t numActiveLights = 0;
	auto view = this->GetAllEntitiesWith<LightComponent, TransformComponent>();
	for (auto e : view)
	{
		auto [lightComponent, transformComponent] = view.get<LightComponent, TransformComponent>(e);

		if (lightComponent.IsEnabled())
		{
			numActiveLights++;
		}

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

	this->m_shaderData.LightCount = numActiveLights;

	auto lightView = this->GetAllEntitiesWith<LightComponent>();
	if (!this->m_lightBuffer.IsValid() || 
		(RHI::IGraphicsDevice::GPtr->GetBufferDesc(this->m_lightBuffer).SizeInBytes / sizeof(Shader::New::Light)) < lightView.size())
	{
		// Create Buffers
		RHI::BufferDesc desc = {};
		desc.Binding = BindingFlags::ShaderResource;
		desc.InitialState = ResourceStates::ShaderResource;
		desc.MiscFlags = BufferMiscFlags::Structured | BufferMiscFlags::Bindless;
		desc.SizeInBytes = sizeof(Shader::New::Light) * (lightView.size() == 0 ? 10 : lightView.size());
		desc.StrideInBytes = sizeof(Shader::New::Light);
		desc.DebugName = "Light Buffer";

		if (this->m_lightBuffer.IsValid())
		{
			RHI::IGraphicsDevice::GPtr->DeleteBuffer(this->m_lightBuffer);
		}
		this->m_lightBuffer = RHI::IGraphicsDevice::GPtr->CreateBuffer(desc);
		this->m_shaderData.LightBufferIdx = RHI::IGraphicsDevice::GPtr->GetDescriptorIndex(this->m_lightBuffer, SubresouceType::SRV);

		desc.CreateBindless = false;
		desc.DebugName = "Light Upload Data";
		desc.Usage = RHI::Usage::Upload;
		desc.Binding = RHI::BindingFlags::None;
		desc.MiscFlags = RHI::BufferMiscFlags::None;
		desc.InitialState = ResourceStates::CopySource;
		for (int i = 0; i < this->m_lightUploadBuffers.size(); i++)
		{
			if (this->m_lightUploadBuffers[i].IsValid())
			{
				RHI::IGraphicsDevice::GPtr->DeleteBuffer(this->m_lightUploadBuffers[i]);
			}
			this->m_lightUploadBuffers[i] = RHI::IGraphicsDevice::GPtr->CreateBuffer(desc);
		}
	}

	if (numActiveLights == 0)
	{
		return;
	}


	// Fill Active Buffer
	Shader::New::Light* pBufferData = (Shader::New::Light*)IGraphicsDevice::GPtr->GetBufferMappedData(this->GetLightUploadBuffer());
	uint32_t currLight = 0;
	for (auto entity : lightView)
	{
		auto& light = view.get<LightComponent>(entity);
		if (!light.IsEnabled())
		{
			continue;
		}

		Shader::New::Light* shaderData = pBufferData + currLight;
		*shaderData = {};

		shaderData->SetType(light.Type);
		shaderData->Position = light.Position;
		shaderData->SetRange(light.Range);
		
		shaderData->SetColor({ light.Colour.x * light.Intensity, light.Colour.y * light.Intensity, light.Colour.z * light.Intensity, 1 });

		switch (light.Type)
		{
		case LightComponent::kDirectionalLight:
		{
			shaderData->SetDirection(light.Direction);
			break;
		}

		case LightComponent::kSpotLight:
		{
			const float outerConeAngle = light.OuterConeAngle;
			const float innerConeAngle = std::min(light.InnerConeAngle, outerConeAngle);
			const float outerConeAngleCos = std::cos(outerConeAngle);
			const float innerConeAngleCos = std::cos(innerConeAngle);

			// https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual#inner-and-outer-cone-angles
			const float lightAngleScale = 1.0f / std::max(0.001f, innerConeAngleCos - outerConeAngleCos);
			const float lightAngleOffset = -outerConeAngleCos * lightAngleScale;

			shaderData->SetConeAngleCos(outerConeAngleCos);
			shaderData->SetAngleScale(lightAngleScale);
			shaderData->SetAngleOffset(lightAngleOffset);
			shaderData->SetDirection(light.Direction);
			break;
		}

		case LightComponent::kOmniLight:
		default:
		{
			// No-op
		}
		}
		light.GlobalBufferIndex = currLight++;
	}
	this->m_isDirtyLights = false;
}

void PhxEngine::Scene::Scene::RunMeshInstanceUpdateSystem()
{
	auto instanceView = this->GetAllEntitiesWith<MeshInstanceComponent>();

	const size_t instanceBufferSizeInBytes = sizeof(Shader::New::ObjectInstance) * instanceView.size();
	if (!this->m_instanceGpuBuffer.IsValid() ||
		(RHI::IGraphicsDevice::GPtr->GetBufferDesc(this->m_instanceGpuBuffer).SizeInBytes) < instanceBufferSizeInBytes)
	{
		RHI::BufferDesc desc = {};
		desc.DebugName = "Instance Data";
		desc.Binding = RHI::BindingFlags::ShaderResource;
		desc.InitialState = ResourceStates::ShaderResource;
		desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Structured;
		desc.CreateBindless = true;
		desc.StrideInBytes = sizeof(Shader::New::ObjectInstance);
		desc.SizeInBytes = instanceBufferSizeInBytes;

		if (this->m_instanceGpuBuffer.IsValid())
		{
			IGraphicsDevice::GPtr->DeleteBuffer(this->m_instanceGpuBuffer);
		}
		this->m_instanceGpuBuffer = IGraphicsDevice::GPtr->CreateBuffer(desc);
		this->m_shaderData.ObjectBufferIdx = RHI::IGraphicsDevice::GPtr->GetDescriptorIndex(this->m_instanceGpuBuffer, SubresouceType::SRV);

		desc.CreateBindless = false;
		desc.DebugName = "Instance Upload Data";
		desc.Usage = RHI::Usage::Upload;
		desc.Binding = RHI::BindingFlags::None;
		desc.MiscFlags = RHI::BufferMiscFlags::None;
		desc.InitialState = ResourceStates::CopySource;
		for (int i = 0; i < this->m_instanceUploadBuffers.size(); i++)
		{
			if (this->m_instanceUploadBuffers[i].IsValid())
			{
				RHI::IGraphicsDevice::GPtr->DeleteBuffer(this->m_instanceUploadBuffers[i]);
			}
			this->m_instanceUploadBuffers[i] = RHI::IGraphicsDevice::GPtr->CreateBuffer(desc);
		}
	}

	this->BuildIndirectBuffers(RHI::IGraphicsDevice::GPtr);

	Shader::New::ObjectInstance* instancePtr = (Shader::New::ObjectInstance*)IGraphicsDevice::GPtr->GetBufferMappedData(this->GetInstanceUploadBuffer());
	uint32_t currInstanceIndex = 0;
	auto instanceTransformView = this->GetAllEntitiesWith<MeshInstanceComponent, TransformComponent>();
	for (auto e : instanceTransformView)
	{
		auto [meshInstanceComponent, transformComponent] = instanceTransformView.get<MeshInstanceComponent, TransformComponent>(e);

		Shader::New::ObjectInstance* shaderMeshInstance = instancePtr + currInstanceIndex;

		auto& mesh = this->GetRegistry().get<MeshComponent>(meshInstanceComponent.Mesh);
		shaderMeshInstance->WorldMatrix = transformComponent.WorldMatrix;
		shaderMeshInstance->GeometryIndex = mesh.GlobalIndexOffsetGeometryBuffer;
		shaderMeshInstance->MeshletOffset = mesh.GlobalOffsetMeshletBuffer;
		shaderMeshInstance->Colour = Core::Math::PackColour(meshInstanceComponent.Color);
		shaderMeshInstance->Emissive = Core::Math::PackColour(meshInstanceComponent.EmissiveColor);
		meshInstanceComponent.GlobalBufferIndex = currInstanceIndex++;
	}

	this->m_shaderData.InstanceCount = instanceView.size();
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

	for (int i = 0; i < this->m_lightUploadBuffers.size(); i++)
	{
		if (this->m_lightUploadBuffers[i].IsValid())
		{
			IGraphicsDevice::GPtr->DeleteBuffer(this->m_lightUploadBuffers[i]);
		}
	}

	for (int i = 0; i < this->m_instanceUploadBuffers.size(); i++)
	{
		if (this->m_instanceUploadBuffers[i].IsValid())
		{
			IGraphicsDevice::GPtr->DeleteBuffer(this->m_instanceUploadBuffers[i]);
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

void PhxEngine::Scene::Scene::OnConstructOrUpdate(entt::registry& registry, entt::entity entity)
{
	if (registry.try_get<MeshInstanceComponent>(entity))
	{
		this->m_isDirtyMeshInstances = true;
	}

	if (registry.try_get<LightComponent>(entity))
	{
		this->m_isDirtyLights = true;
	}
}


void PhxEngine::Scene::Scene::BuildGeometryData(RHI::ICommandList* commandList, RHI::IGraphicsDevice* gfxDevice, std::vector<Renderer::ResourceUpload>& resourcesToFree)
{
	auto meshView = this->GetAllEntitiesWith<MeshComponent>();
	const size_t geometryBufferSize = sizeof(Shader::New::Geometry) * meshView.size();

	const size_t geometryBoundsSize = sizeof(DirectX::XMFLOAT4) * meshView.size();

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

	desc.DebugName = "Geometry Bounds Data";
	desc.StrideInBytes = sizeof(DirectX::XMFLOAT4);
	desc.SizeInBytes = geometryBoundsSize;

	if (this->m_geometryBoundsGpuBuffer.IsValid())
	{
		IGraphicsDevice::GPtr->DeleteBuffer(this->m_geometryBoundsGpuBuffer);
	}
	this->m_geometryBoundsGpuBuffer = IGraphicsDevice::GPtr->CreateBuffer(desc);

	Renderer::ResourceUpload geometryUploadBuffer = Renderer::CreateResourceUpload(geometryBufferSize);
	Shader::New::Geometry* geometryBufferMappedData = (Shader::New::Geometry*)geometryUploadBuffer.Data;

	Renderer::ResourceUpload geometryBoundsUploadBuffer = Renderer::CreateResourceUpload(geometryBoundsSize);
	DirectX::XMFLOAT4* geometryBoundsBufferMappedData = (DirectX::XMFLOAT4*)geometryBoundsUploadBuffer.Data;

	uint32_t currGeoIndex = 0;
	for (auto entity : meshView)
	{
		auto& mesh = meshView.get<MeshComponent>(entity);
		mesh.GlobalIndexOffsetGeometryBuffer = currGeoIndex++;

		Shader::New::Geometry* geometryShaderData = geometryBufferMappedData + mesh.GlobalIndexOffsetGeometryBuffer;
		auto& material = this->GetRegistry().get<MaterialComponent>(mesh.Material);
		geometryShaderData->MaterialIndex = material.GlobalBufferIndex;
		geometryShaderData->DrawFlags = 0;
		if (material.BlendMode == PhxEngine::Renderer::BlendMode::Alpha)
		{
			geometryShaderData->DrawFlags |= Shader::New::DRAW_FLAGS_TRANSPARENT;
		}

		geometryShaderData->NumIndices = mesh.TotalIndices;
		geometryShaderData->NumVertices = mesh.TotalVertices;
		geometryShaderData->IndexOffset = mesh.GlobalOffsetIndexBuffer;
		geometryShaderData->MeshletOffset = mesh.GlobalOffsetMeshletBuffer;
		geometryShaderData->MeshletCount = mesh.Meshlets.size();
		geometryShaderData->MeshletPrimtiveOffset = mesh.GlobalOffsetMeshletPrimitiveBuffer;
		geometryShaderData->MeshletUniqueVertexIBOffset = mesh.GlobalOffsetUnqiueVertexIBBuffer / sizeof(uint32_t);

		geometryShaderData->VertexBufferIndex = gfxDevice->GetDescriptorIndex(this->m_globalVertexBuffer, RHI::SubresouceType::SRV);
		geometryShaderData->PositionOffset = mesh.GlobalByteOffsetVertexBuffer + mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Position).ByteOffset;
		geometryShaderData->TexCoordOffset = mesh.GlobalByteOffsetVertexBuffer + mesh.GetVertexAttribute(MeshComponent::VertexAttribute::TexCoord).ByteOffset;
		geometryShaderData->NormalOffset = mesh.GlobalByteOffsetVertexBuffer + mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Normal).ByteOffset;
		geometryShaderData->TangentOffset = mesh.GlobalByteOffsetVertexBuffer + mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Tangent).ByteOffset;

		static_assert(sizeof(BoundingSphere) == sizeof(DirectX::XMFLOAT4));
		DirectX::XMFLOAT4* boundsData = geometryBoundsBufferMappedData + mesh.GlobalIndexOffsetGeometryBuffer;
		boundsData->x = mesh.BoundingSphere.Centre.x;
		boundsData->y = mesh.BoundingSphere.Centre.y;
		boundsData->z = mesh.BoundingSphere.Centre.z;
		boundsData->w = mesh.BoundingSphere.Radius;
		
	}

	// Do meshlet data
	RHI::GpuBarrier preCopyBarriers[] =
	{
		RHI::GpuBarrier::CreateBuffer(this->m_geometryGpuBuffer, ResourceStates::ShaderResource, ResourceStates::CopyDest),
		RHI::GpuBarrier::CreateBuffer(this->m_geometryBoundsGpuBuffer, ResourceStates::ShaderResource, ResourceStates::CopyDest),
	};
	commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));

	commandList->CopyBuffer(
		this->m_geometryGpuBuffer,
		0,
		geometryUploadBuffer.UploadBuffer,
		0,
		geometryBufferSize);

	commandList->CopyBuffer(
		this->m_geometryBoundsGpuBuffer,
		0,
		geometryBoundsUploadBuffer.UploadBuffer,
		0,
		geometryBoundsSize);

	RHI::GpuBarrier postCopyBarriers[] =
	{
		RHI::GpuBarrier::CreateBuffer(this->m_geometryGpuBuffer, ResourceStates::CopyDest, ResourceStates::ShaderResource),
		RHI::GpuBarrier::CreateBuffer(this->m_geometryBoundsGpuBuffer, ResourceStates::CopyDest, ResourceStates::ShaderResource),
	};

	commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postCopyBarriers, _countof(postCopyBarriers)));
	
	resourcesToFree.push_back(geometryUploadBuffer);
	resourcesToFree.push_back(geometryBoundsUploadBuffer);
}

void PhxEngine::Scene::Scene::BuildIndirectBuffers(RHI::IGraphicsDevice* gfxDevice)
{
	auto view = this->GetAllEntitiesWith<MeshInstanceComponent>();

	const size_t indirectMeshBufferByteSize = Core::Helpers::AlignUp(sizeof(Shader::New::MeshDrawCommand) * view.size(), gfxDevice->GetUavCounterPlacementAlignment()) + sizeof(uint32_t);

	if (!this->m_indirectDrawEarlyMeshBuffer.IsValid() ||
		gfxDevice->GetBufferDesc(this->m_indirectDrawEarlyMeshBuffer).SizeInBytes < indirectMeshBufferByteSize)
	{
		if (this->m_indirectDrawEarlyMeshBuffer.IsValid())
		{
			gfxDevice->DeleteBuffer(this->m_indirectDrawEarlyMeshBuffer);
		}

		this->m_indirectDrawEarlyMeshBuffer = gfxDevice->CreateBuffer({
				   .MiscFlags = BufferMiscFlags::Structured | BufferMiscFlags::HasCounter | BufferMiscFlags::Bindless,
				   .Binding = BindingFlags::UnorderedAccess,
				   .InitialState = ResourceStates::IndirectArgument,
				   .StrideInBytes = sizeof(Shader::New::MeshDrawCommand),
				   .SizeInBytes = indirectMeshBufferByteSize,
				   .AllowUnorderedAccess = true,
				   .UavCounterOffsetInBytes = indirectMeshBufferByteSize - sizeof(uint32_t),
				   .DebugName = "Indirect Draw Early (Mesh)" });

	}

	if (!this->m_indirectDrawEarlyTransparentMeshBuffer.IsValid() ||
		gfxDevice->GetBufferDesc(this->m_indirectDrawEarlyTransparentMeshBuffer).SizeInBytes < indirectMeshBufferByteSize)
	{
		if (this->m_indirectDrawEarlyTransparentMeshBuffer.IsValid())
		{
			gfxDevice->DeleteBuffer(this->m_indirectDrawEarlyTransparentMeshBuffer);
		}

		this->m_indirectDrawEarlyTransparentMeshBuffer = gfxDevice->CreateBuffer({
				   .MiscFlags = BufferMiscFlags::Structured | BufferMiscFlags::HasCounter | BufferMiscFlags::Bindless,
				   .Binding = BindingFlags::UnorderedAccess,
				   .InitialState = ResourceStates::IndirectArgument,
				   .StrideInBytes = sizeof(Shader::New::MeshDrawCommand),
				   .SizeInBytes = indirectMeshBufferByteSize,
				   .AllowUnorderedAccess = true,
				   .UavCounterOffsetInBytes = indirectMeshBufferByteSize - sizeof(uint32_t),
				   .DebugName = "Indirect Draw Early (Transparent Mesh)" });

	}

	const size_t indirectMeshletBufferByteSize = Core::Helpers::AlignUp(sizeof(Shader::New::MeshletDrawCommand) * view.size(), gfxDevice->GetUavCounterPlacementAlignment()) + sizeof(uint32_t);

	if (!this->m_indirectDrawEarlyMeshletBuffer.IsValid() ||
		gfxDevice->GetBufferDesc(this->m_indirectDrawEarlyMeshletBuffer).SizeInBytes < indirectMeshletBufferByteSize)
	{
		if (this->m_indirectDrawEarlyMeshletBuffer.IsValid())
		{
			gfxDevice->DeleteBuffer(this->m_indirectDrawEarlyMeshletBuffer);
		}
		this->m_indirectDrawEarlyMeshletBuffer = gfxDevice->CreateBuffer({
				   .MiscFlags = BufferMiscFlags::Structured | BufferMiscFlags::HasCounter | BufferMiscFlags::Bindless,
				   .Binding = BindingFlags::UnorderedAccess,
				   .InitialState = ResourceStates::IndirectArgument,
				   .StrideInBytes = sizeof(Shader::New::MeshDrawCommand),
				   .SizeInBytes = indirectMeshletBufferByteSize,
				   .AllowUnorderedAccess = true,
				   .UavCounterOffsetInBytes = indirectMeshletBufferByteSize - sizeof(uint32_t),
				   .DebugName = "Indirect Draw Early (Meshlet)" });
	}

	if (!this->m_indirectDrawEarlyTransparentMeshBuffer.IsValid() ||
		gfxDevice->GetBufferDesc(this->m_indirectDrawEarlyTransparentMeshBuffer).SizeInBytes < indirectMeshletBufferByteSize)
	{
		if (this->m_indirectDrawEarlyTransparentMeshBuffer.IsValid())
		{
			gfxDevice->DeleteBuffer(this->m_indirectDrawEarlyTransparentMeshBuffer);
		}
		this->m_indirectDrawEarlyTransparentMeshBuffer = gfxDevice->CreateBuffer({
				   .MiscFlags = BufferMiscFlags::Structured | BufferMiscFlags::HasCounter | BufferMiscFlags::Bindless,
				   .Binding = BindingFlags::UnorderedAccess,
				   .InitialState = ResourceStates::IndirectArgument,
				   .StrideInBytes = sizeof(Shader::New::MeshDrawCommand),
				   .SizeInBytes = indirectMeshletBufferByteSize,
				   .AllowUnorderedAccess = true,
				   .UavCounterOffsetInBytes = indirectMeshletBufferByteSize - sizeof(uint32_t),
				   .DebugName = "Indirect Draw Early (Transparent Meshlet)" });
	}

	if (!this->m_culledInstancesCounterBuffer.IsValid())
	{
		this->m_culledInstancesCounterBuffer = gfxDevice->CreateBuffer({
				   .MiscFlags = BufferMiscFlags::Raw | BufferMiscFlags::Bindless,
				   .Binding = BindingFlags::UnorderedAccess | BindingFlags::ShaderResource,
				   .InitialState = ResourceStates::ShaderResource,
				   .StrideInBytes = sizeof(uint32_t),
				   .SizeInBytes = sizeof(uint32_t),
				   .AllowUnorderedAccess = true });
	}

	const size_t cullInstanceBufferSizeInBytes = sizeof(Shader::New::MeshDrawCommand) * view.size();
	if (!this->m_culledInstancesBuffer.IsValid() ||
		gfxDevice->GetBufferDesc(this->m_culledInstancesBuffer).SizeInBytes < cullInstanceBufferSizeInBytes)
	{
		if (this->m_culledInstancesBuffer.IsValid())
		{
			gfxDevice->DeleteBuffer(this->m_culledInstancesBuffer);
		}
		this->m_culledInstancesBuffer = gfxDevice->CreateBuffer({
				   .MiscFlags = BufferMiscFlags::Structured | BufferMiscFlags::HasCounter | BufferMiscFlags::Bindless,
				   .Binding = BindingFlags::UnorderedAccess | BindingFlags::ShaderResource,
				   .InitialState = ResourceStates::ShaderResource,
				   .StrideInBytes = sizeof(Shader::New::MeshDrawCommand),
				   .SizeInBytes = cullInstanceBufferSizeInBytes,
				   .AllowUnorderedAccess = true,
				   .UavCounterOffsetInBytes = 0,
				   .UavCounterBuffer = this->m_culledInstancesCounterBuffer });
	}


	if (!this->m_indirectDrawLateBuffer.IsValid() ||
		gfxDevice->GetBufferDesc(this->m_indirectDrawLateBuffer).SizeInBytes < indirectMeshBufferByteSize)
	{
		if (this->m_indirectDrawLateBuffer.IsValid())
		{
			gfxDevice->DeleteBuffer(this->m_indirectDrawLateBuffer);
		}
		this->m_indirectDrawLateBuffer = gfxDevice->CreateBuffer({
				   .MiscFlags = BufferMiscFlags::Structured | BufferMiscFlags::HasCounter | BufferMiscFlags::Bindless,
				   .Binding = BindingFlags::UnorderedAccess,
				   .InitialState = ResourceStates::IndirectArgument,
				   .StrideInBytes = sizeof(Shader::New::MeshDrawCommand),
				   .SizeInBytes = indirectMeshBufferByteSize,
				   .AllowUnorderedAccess = true,
				   .UavCounterOffsetInBytes = indirectMeshBufferByteSize - sizeof(uint32_t) });
	}
}

void PhxEngine::Scene::Scene::BuildSceneData(RHI::ICommandList* commandList, RHI::IGraphicsDevice* gfxDevice)
{
	this->m_shaderData = {};
	this->m_shaderData.ObjectBufferIdx = RHI::cInvalidDescriptorIndex;;
	this->m_shaderData.GeometryBufferIdx = gfxDevice->GetDescriptorIndex(this->m_geometryGpuBuffer, SubresouceType::SRV);
	this->m_shaderData.GeometryBoundsBufferIdx = gfxDevice->GetDescriptorIndex(this->m_geometryBoundsGpuBuffer, SubresouceType::SRV);
	this->m_shaderData.MaterialBufferIdx = gfxDevice->GetDescriptorIndex(this->m_materialGpuBuffer, SubresouceType::SRV);
	this->m_shaderData.GlobalVertexBufferIdx = gfxDevice->GetDescriptorIndex(this->m_globalVertexBuffer, SubresouceType::SRV);
	this->m_shaderData.GlobalIndexBufferIdx = gfxDevice->GetDescriptorIndex(this->m_globalIndexBuffer, SubresouceType::SRV);
	this->m_shaderData.MeshletCullDataBufferIdx = gfxDevice->GetDescriptorIndex(this->m_globalMeshletCullDataBuffer, SubresouceType::SRV);
	this->m_shaderData.MeshletBufferIdx = gfxDevice->GetDescriptorIndex(this->m_globalMeshletBuffer, SubresouceType::SRV);
	this->m_shaderData.MeshletPrimitiveIdx = gfxDevice->GetDescriptorIndex(this->m_globalMeshletPrimitiveBuffer, SubresouceType::SRV);
	this->m_shaderData.UniqueVertexIBIdx = gfxDevice->GetDescriptorIndex(this->m_globalUniqueVertexIBBuffer, SubresouceType::SRV);
	this->m_shaderData.IndirectEarlyMeshBufferIdx = gfxDevice->GetDescriptorIndex(this->m_indirectDrawEarlyMeshBuffer, SubresouceType::UAV);
	this->m_shaderData.IndirectEarlyMeshletBufferIdx = gfxDevice->GetDescriptorIndex(this->m_indirectDrawEarlyMeshletBuffer, SubresouceType::UAV);
	this->m_shaderData.IndirectEarlyTransparentMeshBufferIdx = gfxDevice->GetDescriptorIndex(this->m_indirectDrawEarlyTransparentMeshBuffer, SubresouceType::UAV);
	this->m_shaderData.IndirectEarlyTransparentMeshletBufferIdx = gfxDevice->GetDescriptorIndex(this->m_indirectDrawEarlyTransparentMeshletBuffer, SubresouceType::UAV);
	this->m_shaderData.IndirectLateBufferIdx = gfxDevice->GetDescriptorIndex(this->m_indirectDrawLateBuffer, SubresouceType::UAV);
	this->m_shaderData.CulledInstancesBufferUavIdx = gfxDevice->GetDescriptorIndex(this->m_culledInstancesBuffer, SubresouceType::UAV);
	this->m_shaderData.CulledInstancesBufferSrvIdx = gfxDevice->GetDescriptorIndex(this->m_culledInstancesBuffer, SubresouceType::SRV);
	this->m_shaderData.CulledInstancesCounterBufferIdx = gfxDevice->GetDescriptorIndex(this->m_culledInstancesCounterBuffer, SubresouceType::SRV);
	this->m_shaderData.LightCount = 0;
	this->m_shaderData.LightBufferIdx = RHI::cInvalidDescriptorIndex;
	this->m_shaderData.InstanceCount = 0;

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

	commandList->TransitionBarrier(this->m_materialGpuBuffer, ResourceStates::ShaderResource, ResourceStates::CopyDest);
	commandList->CopyBuffer(
		this->m_materialGpuBuffer,
		0,
		uploadBuffer.UploadBuffer,
		0,
		mtlBufferSize);

	commandList->TransitionBarrier(this->m_materialGpuBuffer, ResourceStates::CopyDest, ResourceStates::ShaderResource);
}

void PhxEngine::Scene::Scene::BuildMeshData(RHI::ICommandList* commandList, RHI::IGraphicsDevice* gfxDevice)
{
	// Construct Mesh Render Data
	auto meshView = this->GetAllEntitiesWith<MeshComponent>();
	size_t totalVertexByteCount = 0;
	size_t totalIndexCount = 0;
	size_t totalMeshletCount = 0;
	size_t totalUniqueVertexIBCount = 0;
	size_t totalMeshletPrimitiveCount = 0;
	for (auto e : meshView)
	{
		auto& mesh = meshView.get<MeshComponent>(e);

		mesh.GlobalOffsetIndexBuffer = totalIndexCount;
		mesh.GlobalByteOffsetVertexBuffer = totalVertexByteCount;

		mesh.GlobalOffsetMeshletBuffer = totalMeshletCount;
		mesh.GlobalOffsetUnqiueVertexIBBuffer = totalUniqueVertexIBCount;
		mesh.GlobalOffsetMeshletPrimitiveBuffer = totalMeshletPrimitiveCount;
		mesh.GlobalOffsetMeshletCullDataBuffer = totalMeshletCount;

		assert(mesh.VertexBuffer.IsValid());
		totalIndexCount += mesh.TotalIndices;
		totalVertexByteCount += gfxDevice->GetBufferDesc(mesh.VertexBuffer).SizeInBytes;

		totalMeshletCount += mesh.Meshlets.size();
		totalUniqueVertexIBCount += mesh.UniqueVertexIB.size();
		totalMeshletPrimitiveCount += mesh.MeshletTriangles.size();
	}

	this->m_globalIndexBuffer = gfxDevice->CreateIndexBuffer({
			.StrideInBytes = sizeof(uint32_t),
			.SizeInBytes = totalIndexCount * sizeof(uint32_t),
			.DebugName = "Scene Index Buffer" });

	this->m_globalVertexBuffer = gfxDevice->CreateIndexBuffer({
			.MiscFlags = RHI::BufferMiscFlags::Raw | RHI::BufferMiscFlags::Bindless,
			.Binding = RHI::BindingFlags::VertexBuffer | RHI::BindingFlags::ShaderResource,
			.StrideInBytes = sizeof(float),
			.SizeInBytes = totalVertexByteCount,
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
			.StrideInBytes = sizeof(uint8_t),
			.SizeInBytes = totalUniqueVertexIBCount * sizeof(uint8_t),
			.DebugName = "Scene Meshlet Unique Vertex IB" });
	const size_t uniqueVertexIBStride= gfxDevice->GetBufferDesc(this->m_globalUniqueVertexIBBuffer).StrideInBytes;

	this->m_globalMeshletPrimitiveBuffer = gfxDevice->CreateBuffer({
			.MiscFlags = RHI::BufferMiscFlags::Structured | RHI::BufferMiscFlags::Bindless,
			.Binding = RHI::BindingFlags::ShaderResource,
			.StrideInBytes = sizeof(DirectX::MeshletTriangle),
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
			mesh.GlobalOffsetIndexBuffer * sizeof(uint32_t),
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
			mesh.GlobalOffsetMeshletBuffer * meshletBufferStride,
			mesh.MeshletBuffer,
			0,
			gfxDevice->GetBufferDesc(mesh.MeshletBuffer).SizeInBytes);

		commandList->CopyBuffer(
			this->m_globalUniqueVertexIBBuffer,
			mesh.GlobalOffsetUnqiueVertexIBBuffer * uniqueVertexIBStride,
			mesh.UniqueVertexIBBuffer,
			0,
			gfxDevice->GetBufferDesc(mesh.UniqueVertexIBBuffer).SizeInBytes);

		commandList->CopyBuffer(
			this->m_globalMeshletPrimitiveBuffer,
			mesh.GlobalOffsetMeshletPrimitiveBuffer * meshletPrimtiveStide,
			mesh.MeshletPrimitivesBuffer,
			0,
			gfxDevice->GetBufferDesc(mesh.MeshletPrimitivesBuffer).SizeInBytes);

		commandList->CopyBuffer(
			this->m_globalMeshletCullDataBuffer,
			mesh.GlobalOffsetMeshletCullDataBuffer * meshletCullDataStride,
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


Entity PhxEngine::Scene::Scene::CreateCube(
	RHI::IGraphicsDevice* gfxDevice,
	entt::entity matId,
	float size,
	bool rhsCoord)
{
	// A cube has six faces, each one pointing in a different direction.
	constexpr int FaceCount = 6;

	constexpr XMVECTORF32 faceNormals[FaceCount] =
	{
		{ 0,  0,  1 },
		{ 0,  0, -1 },
		{ 1,  0,  0 },
		{ -1,  0,  0 },
		{ 0,  1,  0 },
		{ 0, -1,  0 },
	};

	constexpr XMFLOAT3 faceColour[] =
	{
		{ 1.0f,  0.0f,  0.0f },
		{ 0.0f,  1.0f,  0.0f },
		{ 0.0f,  0.0f,  1.0f },
	};

	constexpr XMFLOAT2 textureCoordinates[4] =
	{
		{ 1, 0 },
		{ 1, 1 },
		{ 0, 1 },
		{ 0, 0 },
	};


	Entity retVal = this->CreateEntity("Cube Mesh");
	MeshComponent& mesh = retVal.AddComponent<MeshComponent>();
	mesh.Material = matId;

	size /= 2;
	mesh.TotalVertices = FaceCount * 4;
	mesh.TotalIndices = FaceCount * 6;

	mesh.InitializeCpuBuffers(this->GetAllocator());

	size_t vbase = 0;
	size_t ibase = 0;
	for (int i = 0; i < FaceCount; i++)
	{
		XMVECTOR normal = faceNormals[i];

		// Get two vectors perpendicular both to the face normal and to each other.
		XMVECTOR basis = (i >= 4) ? g_XMIdentityR2 : g_XMIdentityR1;

		XMVECTOR side1 = XMVector3Cross(normal, basis);
		XMVECTOR side2 = XMVector3Cross(normal, side1);

		// Six indices (two triangles) per face.
		mesh.Indices[ibase + 0] = static_cast<uint16_t>(vbase + 0);
		mesh.Indices[ibase + 1] = static_cast<uint16_t>(vbase + 1);
		mesh.Indices[ibase + 2] = static_cast<uint16_t>(vbase + 2);

		mesh.Indices[ibase + 3] = static_cast<uint16_t>(vbase + 0);
		mesh.Indices[ibase + 4] = static_cast<uint16_t>(vbase + 2);
		mesh.Indices[ibase + 5] = static_cast<uint16_t>(vbase + 3);

		XMFLOAT3 positon;
		XMStoreFloat3(&positon, (normal - side1 - side2) * size);
		mesh.Positions[vbase + 0] = positon;

		XMStoreFloat3(&positon, (normal - side1 + side2) * size);
		mesh.Positions[vbase + 1] = positon;

		XMStoreFloat3(&positon, (normal + side1 + side2) * size);
		mesh.Positions[vbase + 2] = positon;

		XMStoreFloat3(&positon, (normal + side1 - side2) * size);
		mesh.Positions[vbase + 3] = positon;

		mesh.TexCoords[vbase + 0] = textureCoordinates[0];
		mesh.TexCoords[vbase + 1] = textureCoordinates[1];
		mesh.TexCoords[vbase + 2] = textureCoordinates[2];
		mesh.TexCoords[vbase + 3] = textureCoordinates[3];

		mesh.Colour[vbase + 0] = faceColour[0];
		mesh.Colour[vbase + 1] = faceColour[1];
		mesh.Colour[vbase + 2] = faceColour[2];
		mesh.Colour[vbase + 3] = faceColour[3];

		DirectX::XMStoreFloat3((mesh.Normals + vbase + 0), normal);
		DirectX::XMStoreFloat3((mesh.Normals + vbase + 1), normal);
		DirectX::XMStoreFloat3((mesh.Normals + vbase + 2), normal);
		DirectX::XMStoreFloat3((mesh.Normals + vbase + 3), normal);

		vbase += 4;
		ibase += 6;
	}

	mesh.Flags = 
		MeshComponent::Flags::kContainsNormals |
		MeshComponent::Flags::kContainsTexCoords |
		MeshComponent::Flags::kContainsTangents |
		MeshComponent::Flags::kContainsColour;

	mesh.ComputeTangentSpace();

	if (rhsCoord)
	{
		mesh.ReverseWinding();
		mesh.FlipZ();
	}

	// Calculate AABB
	mesh.ComputeBounds();
	mesh.ComputeMeshletData(this->GetAllocator());

	mesh.BuildRenderData(this->GetAllocator(), gfxDevice);
	return retVal;
}

Entity PhxEngine::Scene::Scene::CreateSphere(
	RHI::IGraphicsDevice* gfxDevice,
	entt::entity matId,
	float diameter,
	size_t tessellation,
	bool rhcoords)
{
	if (tessellation < 3)
	{
		LOG_CORE_ERROR("tessellation parameter out of range");
		throw std::out_of_range("tessellation parameter out of range");
	}

	Entity retVal = this->CreateEntity("Cube Mesh");
	MeshComponent& mesh = retVal.AddComponent<MeshComponent>();
	mesh.Material = matId;

	float radius = diameter / 2.0f;
	size_t verticalSegments = tessellation;
	size_t horizontalSegments = tessellation * 2;

	mesh.TotalVertices = (verticalSegments + 1) * (horizontalSegments + 1);
	mesh.TotalIndices += verticalSegments * (horizontalSegments + 1) * 6;
	mesh.InitializeCpuBuffers(this->GetAllocator());

	// Create rings of vertices at progressively higher latitudes.
	size_t vbase = 0;
	for (size_t i = 0; i <= verticalSegments; i++)
	{
		float v = 1 - (float)i / verticalSegments;

		float latitude = (i * XM_PI / verticalSegments) - XM_PIDIV2;
		float dy, dxz;

		XMScalarSinCos(&dy, &dxz, latitude);

		// Create a single ring of vertices at this latitude.
		for (size_t j = 0; j <= horizontalSegments; j++)
		{
			float u = (float)j / horizontalSegments;

			float longitude = j * XM_2PI / horizontalSegments;
			float dx, dz;

			XMScalarSinCos(&dx, &dz, longitude);

			dx *= dxz;
			dz *= dxz;

			XMVECTOR normal = XMVectorSet(dx, dy, dz, 0);

			XMFLOAT3 positon;
			XMStoreFloat3(&positon, normal * radius);
			mesh.Positions[vbase] = positon;
			mesh.TexCoords[vbase] = { u, v };
			mesh.Normals[vbase] = { dx, dy, dz };
			mesh.Colour[vbase] = { 0.1f, 0.1f, 0.1f };
			vbase++;
		}
	}

	// Fill the index buffer with triangles joining each pair of latitude rings.
	size_t stride = horizontalSegments + 1;
	size_t iBase = 0;
	for (size_t i = 0; i < verticalSegments; i++)
	{
		for (size_t j = 0; j <= horizontalSegments; j++)
		{
			size_t nextI = i + 1;
			size_t nextJ = (j + 1) % stride;

			mesh.Indices[iBase + 0] = static_cast<uint16_t>(i * stride + j);
			mesh.Indices[iBase + 1] = static_cast<uint16_t>(nextI * stride + j);
			mesh.Indices[iBase + 2] = static_cast<uint16_t>(i * stride + nextJ);

			mesh.Indices[iBase + 3] = static_cast<uint16_t>(i * stride + nextJ);
			mesh.Indices[iBase + 4] = static_cast<uint16_t>(nextI * stride + j);
			mesh.Indices[iBase + 5] = static_cast<uint16_t>(nextI * stride + nextJ);
			iBase += 6;
		}
	}

	mesh.Flags =
		MeshComponent::Flags::kContainsNormals |
		MeshComponent::Flags::kContainsTexCoords |
		MeshComponent::Flags::kContainsTangents |
		MeshComponent::Flags::kContainsColour;

	mesh.ComputeTangentSpace();

	if (rhcoords)
	{
		mesh.ReverseWinding();
		mesh.FlipZ();
	}

	mesh.ComputeBounds();
	mesh.ComputeMeshletData(this->GetAllocator());

	mesh.BuildRenderData(this->GetAllocator(), gfxDevice);
	return retVal;
}

Entity PhxEngine::Scene::Scene::CreatePlane(RHI::IGraphicsDevice* gfxDevice, entt::entity matId, float width, float height, bool rhcoords)
{
	constexpr uint32_t PlaneIndices[] =
	{
		0, 3, 1, 1, 3, 2
	};

	Entity retVal = this->CreateEntity("Plane Mesh");
	MeshComponent& mesh = retVal.AddComponent<MeshComponent>();
	mesh.Material = matId;

	mesh.TotalVertices = 4;
	mesh.TotalIndices = _ARRAYSIZE(PlaneIndices);
	mesh.InitializeCpuBuffers(this->GetAllocator());

	mesh.Positions[0] = { -0.5f * width, 0.0f, 0.5f * height };
	mesh.Positions[1] = { 0.5f * width, 0.0f,  0.5f * height };
	mesh.Positions[2] = { 0.5f * width, 0.0f, -0.5f * height };
	mesh.Positions[3] = { -0.5f * width, 0.0f, -0.5f * height };

	mesh.Normals[0] = { 0.0f, 1.0f, 0.0f };
	mesh.Normals[1] = { 0.0f, 1.0f, 0.0f };
	mesh.Normals[2] = { 0.0f, 1.0f, 0.0f };
	mesh.Normals[3] = { 0.0f, 1.0f, 0.0f };

	mesh.TexCoords[0] = { 0, 0 };
	mesh.TexCoords[1] = { 1, 0 };
	mesh.TexCoords[2] = { 1, 1 };
	mesh.TexCoords[3] = { 0, 1 };

	std::memcpy(mesh.Indices, &PlaneIndices, _ARRAYSIZE(PlaneIndices) * sizeof(uint32_t));


	mesh.ComputeTangentSpace();

	mesh.Flags =
		MeshComponent::Flags::kContainsNormals |
		MeshComponent::Flags::kContainsTexCoords |
		MeshComponent::Flags::kContainsTangents |
		MeshComponent::Flags::kContainsColour;

	if (rhcoords)
	{
		mesh.ReverseWinding();
		mesh.FlipZ();
	}

	mesh.ComputeBounds();
	mesh.ComputeMeshletData(this->GetAllocator());

	mesh.BuildRenderData(this->GetAllocator(), gfxDevice);

	return retVal;
}
