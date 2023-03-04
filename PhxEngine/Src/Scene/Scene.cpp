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

PhxEngine::Scene::Scene::Scene()
	: m_numMeshlets(0)
{
	this->m_materialUploadBuffers.resize(IGraphicsDevice::GPtr->GetMaxInflightFrames());
	this->m_geometryUploadBuffers.resize(IGraphicsDevice::GPtr->GetMaxInflightFrames());
	this->m_instanceUploadBuffers.resize(IGraphicsDevice::GPtr->GetMaxInflightFrames());
	this->m_tlasUploadBuffers.resize(IGraphicsDevice::GPtr->GetMaxInflightFrames());
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

void PhxEngine::Scene::Scene::ConstructRenderData(
	RHI::ICommandList* cmd,
	Renderer::ResourceUpload& indexUploader,
	Renderer::ResourceUpload& vertexUploader)
{
	auto view = this->GetAllEntitiesWith<MeshComponent, NameComponent>();
	uint64_t indexBufferSize = 0;
	uint64_t vertexBufferSize = 0;
	for (auto e : view)
	{
		auto [meshComp, nameComp] = view.get<MeshComponent, NameComponent>(e);
		const uint64_t indexSize = meshComp.GetIndexBufferSizeInBytes();
		const uint64_t vertexSize = meshComp.GetVertexBufferSizeInBytes();;

		indexBufferSize += indexSize;
		vertexBufferSize += vertexSize;
	}

	// Construct Upload Buffers
	indexUploader = Renderer::CreateResourceUpload(indexBufferSize);
	vertexUploader = Renderer::CreateResourceUpload(vertexBufferSize);

	for (auto e : view)
	{
		auto [meshComp, nameComp] = view.get<MeshComponent, NameComponent>(e);
		meshComp.CreateRenderData(cmd, indexUploader, vertexUploader);
	}

	// Construct Global Index buffer
	this->MergeMeshes(cmd);
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
}

void PhxEngine::Scene::Scene::RunMaterialUpdateSystem(std::shared_ptr<Renderer::CommonPasses>& commonPasses)
{
	Shader::MaterialData* pMaterialUploadBufferData = (Shader::MaterialData*)IGraphicsDevice::GPtr->GetBufferMappedData(this->GetMaterialUploadBuffer());
	auto view = this->GetAllEntitiesWith<MaterialComponent>();
	uint32_t currMat = 0;
	for (auto entity : view)
	{
		auto& mat = view.get<MaterialComponent>(entity);

		Shader::MaterialData* shaderData = pMaterialUploadBufferData + currMat;

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
		else
		{
			shaderData->AlbedoTexture = commonPasses->GetWhiteTexDescriptorIndex();
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

void PhxEngine::Scene::Scene::RunMeshUpdateSystem()
{
	auto view = this->GetAllEntitiesWith<MeshComponent>();
	for (auto entity : view)
	{
		auto& meshComponent = view.get<MeshComponent>(entity);
		meshComponent.RenderBucketMask = MeshComponent::RenderType::RenderType_Opaque;
		for (auto& surface : meshComponent.Surfaces)
		{
			const auto& material = this->m_registry.get<MaterialComponent>(surface.Material);
			if (material.BlendMode == Renderer::BlendMode::Alpha)
			{
				meshComponent.RenderBucketMask = MeshComponent::RenderType::RenderType_Transparent;
				break;
			}
		}
	}

	// -- Update data --
	Shader::Geometry* geometryBufferMappedData = (Shader::Geometry*)IGraphicsDevice::GPtr->GetBufferMappedData(this->GetGeometryUploadBuffer());
	uint32_t currGeoIndex = 0;
	for (auto entity : view)
	{
		auto& mesh = view.get<MeshComponent>(entity);
		mesh.GlobalGeometryBufferIndex = currGeoIndex;
		for (int i = 0; i < mesh.Surfaces.size(); i++)
		{
			MeshComponent::SurfaceDesc& surfaceDesc = mesh.Surfaces[i];
			surfaceDesc.GlobalGeometryBufferIndex = currGeoIndex++;

			Shader::Geometry* geometryShaderData = geometryBufferMappedData + surfaceDesc.GlobalGeometryBufferIndex;
			auto& material = this->GetRegistry().get<MaterialComponent>(surfaceDesc.Material);
			geometryShaderData->MaterialIndex = material.GlobalBufferIndex;
			geometryShaderData->NumIndices = surfaceDesc.NumIndices;
			geometryShaderData->NumVertices = surfaceDesc.NumVertices;
			geometryShaderData->MeshletOffset = mesh.MeshletCount;
			geometryShaderData->MeshletCount = ((surfaceDesc.NumIndices / 3) + 1) / Shader::MESHLET_TRIANGLE_COUNT;
			mesh.MeshletCount += geometryShaderData->MeshletCount;
			geometryShaderData->IndexOffset = surfaceDesc.IndexOffsetInMesh;
			geometryShaderData->VertexBufferIndex = IGraphicsDevice::GPtr->GetDescriptorIndex(mesh.VertexGpuBuffer, RHI::SubresouceType::SRV);
			geometryShaderData->PositionOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Position).ByteOffset;
			geometryShaderData->TexCoordOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::TexCoord).ByteOffset;
			geometryShaderData->NormalOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Normal).ByteOffset;
			geometryShaderData->TangentOffset = mesh.GetVertexAttribute(MeshComponent::VertexAttribute::Tangent).ByteOffset;
		}
	}
}

void PhxEngine::Scene::Scene::RunProbeUpdateSystem()
{
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
}

void PhxEngine::Scene::Scene::RunLightUpdateSystem()
{
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
}

void PhxEngine::Scene::Scene::RunMeshInstanceUpdateSystem()
{
	auto view = this->GetAllEntitiesWith<MeshInstanceComponent, TransformComponent>();
	for (auto e : view)
	{
		auto [meshInstanceComponent, transformComponent] = view.get<MeshInstanceComponent, TransformComponent>(e);
		meshInstanceComponent.WorldMatrix = transformComponent.WorldMatrix;
	}

	// -- Update data --
	Shader::MeshInstance* pUploadBufferData = (Shader::MeshInstance*)IGraphicsDevice::GPtr->GetBufferMappedData(this->GetInstanceUploadBuffer());
	uint32_t currInstanceIndex = 0;
	for (auto e : view)
	{
		auto [meshInstanceComponent, transformComponent] = view.get<MeshInstanceComponent, TransformComponent>(e);
		Shader::MeshInstance* shaderMeshInstance = pUploadBufferData + currInstanceIndex;

		auto& mesh = this->GetRegistry().get<MeshComponent>(meshInstanceComponent.Mesh);
		shaderMeshInstance->GeometryOffset = mesh.GlobalGeometryBufferIndex;
		shaderMeshInstance->GeometryCount = (uint)mesh.Surfaces.size();
		shaderMeshInstance->WorldMatrix = transformComponent.WorldMatrix;
		shaderMeshInstance->MeshletOffset = mesh.MeshletCount;
		this->m_numMeshlets += mesh.MeshletCount;

		meshInstanceComponent.GlobalBufferIndex = currInstanceIndex++;
	}

	auto aabbView = this->GetAllEntitiesWith<MeshInstanceComponent, AABBComponent>();
	for (auto e : aabbView)
	{
		auto [meshInstanceComponent, aabbComponent] = aabbView.get<MeshInstanceComponent, AABBComponent>(e);
		auto& mesh = this->GetRegistry().get<MeshComponent>(meshInstanceComponent.Mesh);

		aabbComponent.BoundingData = mesh.Aabb.Transform(DirectX::XMLoadFloat4x4(&meshInstanceComponent.WorldMatrix));
	}

}

void PhxEngine::Scene::Scene::FreeResources()
{
	PhxEngine::RHI::IGraphicsDevice::GPtr->DeleteBuffer(this->m_instanceGpuBuffer);
	PhxEngine::RHI::IGraphicsDevice::GPtr->DeleteBuffer(this->m_geometryGpuBuffer);
	PhxEngine::RHI::IGraphicsDevice::GPtr->DeleteBuffer(this->m_materialGpuBuffer);
	PhxEngine::RHI::IGraphicsDevice::GPtr->DeleteBuffer(this->m_meshletGpuBuffer);

	for (int i = 0; i < this->m_materialUploadBuffers.size(); i++)
	{
		if (this->m_materialUploadBuffers[i].IsValid())
		{
			IGraphicsDevice::GPtr->DeleteBuffer(this->m_materialUploadBuffers[i]);
		}
	}

	for (int i = 0; i < this->m_instanceUploadBuffers.size(); i++)
	{
		if (this->m_instanceUploadBuffers[i].IsValid())
		{
			IGraphicsDevice::GPtr->DeleteBuffer(this->m_instanceUploadBuffers[i]);
		}
	}

	for (int i = 0; i < this->m_geometryUploadBuffers.size(); i++)
	{
		if (this->m_geometryUploadBuffers[i].IsValid())
		{
			IGraphicsDevice::GPtr->DeleteBuffer(this->m_geometryUploadBuffers[i]);
		}
	}
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

	for (int i = 0; i < this->m_envMapRenderPasses.size(); i++)
	{
		if (this->m_envMapRenderPasses[i].IsValid())
		{
			IGraphicsDevice::GPtr->DeleteRenderPass(this->m_envMapRenderPasses[i]);
		}
	}

	if (this->m_envMapDepthBuffer.IsValid())
	{
		IGraphicsDevice::GPtr->DeleteTexture(this->m_envMapDepthBuffer);
	}

	if (this->m_envMapArray.IsValid())
	{
		IGraphicsDevice::GPtr->DeleteTexture(this->m_envMapArray);
	}

	auto meshView = this->GetAllEntitiesWith<MeshComponent>();
	for (auto e : meshView)
	{
		auto& meshComp = meshView.get<MeshComponent>(e);
		if (meshComp.Blas.IsValid())
		{
			IGraphicsDevice::GPtr->DeleteBuffer(meshComp.IndexGpuBuffer);
			IGraphicsDevice::GPtr->DeleteBuffer(meshComp.VertexGpuBuffer);
			IGraphicsDevice::GPtr->DeleteRtAccelerationStructure(meshComp.Blas);
		}
	}
}

void PhxEngine::Scene::Scene::UpdateGpuBufferSizes()
{
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
}

void PhxEngine::Scene::Scene::MergeMeshes(RHI::ICommandList* cmd)
{
	uint64_t indexCount = 0;
	uint64_t indexSizeInBytes = 0;

	auto view = this->GetAllEntitiesWith<MeshComponent>();
	for (auto e : view)
	{
		auto& meshComp = view.get<MeshComponent>(e);

		meshComp.GlobalIndexBufferOffset = indexCount;
		indexCount += meshComp.TotalIndices;
		indexSizeInBytes += meshComp.GetIndexBufferSizeInBytes();
	}

	if (this->GetGlobalIndexBuffer().IsValid())
	{
		IGraphicsDevice::GPtr->DeleteBuffer(this->GetGlobalIndexBuffer());
	}

	RHI::BufferDesc indexBufferDesc = {};
	indexBufferDesc.SizeInBytes = indexSizeInBytes;
	indexBufferDesc.StrideInBytes = sizeof(uint32_t);
	indexBufferDesc.DebugName = "Index Buffer";
	indexBufferDesc.Binding = RHI::BindingFlags::IndexBuffer;
	this->m_globalIndexBuffer = RHI::IGraphicsDevice::GPtr->CreateIndexBuffer(indexBufferDesc);

	// Upload data
	{
		RHI::GpuBarrier preCopyBarriers[] =
		{
			RHI::GpuBarrier::CreateBuffer(this->GetGlobalIndexBuffer(), IGraphicsDevice::GPtr->GetBufferDesc(this->GetGlobalIndexBuffer()).InitialState, RHI::ResourceStates::CopyDest),
		};

		cmd->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));
	}

	for (auto e : view)
	{
		auto& meshComp = view.get<MeshComponent>(e);

		BufferDesc vertexBufferDesc = IGraphicsDevice::GPtr->GetBufferDesc(meshComp.VertexGpuBuffer);
		BufferDesc indexBufferDesc = IGraphicsDevice::GPtr->GetBufferDesc(meshComp.IndexGpuBuffer);
		RHI::GpuBarrier preCopyBarriers[] =
		{
			RHI::GpuBarrier::CreateBuffer(meshComp.IndexGpuBuffer, RHI::ResourceStates::IndexGpuBuffer | RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopySource),
		};

		cmd->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));

		cmd->CopyBuffer(
			this->GetGlobalIndexBuffer(),
			meshComp.GlobalIndexBufferOffset * indexBufferDesc.StrideInBytes,
			meshComp.IndexGpuBuffer,
			0,
			indexBufferDesc.SizeInBytes);

		// Upload data
		RHI::GpuBarrier postCopyBarriers[] =
		{
			RHI::GpuBarrier::CreateBuffer(meshComp.IndexGpuBuffer, RHI::ResourceStates::CopySource, RHI::ResourceStates::IndexGpuBuffer | RHI::ResourceStates::ShaderResource),
		};

		cmd->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postCopyBarriers, _countof(postCopyBarriers)));
	}


	// Upload data
	RHI::GpuBarrier postCopyBarriers[] =
	{
		RHI::GpuBarrier::CreateBuffer(this->GetGlobalIndexBuffer(), RHI::ResourceStates::CopyDest, IGraphicsDevice::GPtr->GetBufferDesc(this->GetGlobalIndexBuffer()).InitialState),
	};

	cmd->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postCopyBarriers, _countof(postCopyBarriers)));
}
