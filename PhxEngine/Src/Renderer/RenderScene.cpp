#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"

#include <PhxEngine/Renderer/RenderScene.h>
#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Shaders/ShaderInteropStructures_NEW.h>

using namespace PhxEngine;
using namespace PhxEngine::RHI;

void PhxEngine::Renderer::RenderScene::Initialize(RHI::IGraphicsDevice* gfxDevice, RHI::ICommandList* commandList, Scene::Scene& scene)
{

	// Create Instance Data

	auto meshInstanceView = scene.GetAllEntitiesWith<Scene::MeshInstanceComponent>();

	RHI::BufferDesc desc = {};
	desc.DebugName = "Instance Data";
	desc.Binding = RHI::BindingFlags::ShaderResource;
	desc.InitialState = ResourceStates::ShaderResource;
	desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Structured;
	desc.CreateBindless = true;
	desc.StrideInBytes = sizeof(Shader::MaterialData);
	desc.SizeInBytes = sizeof(Shader::MaterialData) * materialEntitiesView.size();

	this->ObjectInstancesBuffer = IGraphicsDevice::GPtr->CreateBuffer({
			.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Structured,
			.Binding = RHI::BindingFlags::ShaderResource,
			.InitialState = RHI::ResourceStates::ShaderResource,
			.StrideInBytes = sizeof(Shader::MaterialData),
			.SizeInBytes = sizeof(Shader::MaterialData) * materialEntitiesView.size(),
			.CreateBindless = true,
			.DebugName = "Material Data",
		});
}

void PhxEngine::Renderer::RenderScene::PrepareFrame(RHI::IGraphicsDevice* gfxDevice, RHI::ICommandList* commandList, Scene::Scene& scene)
{
}

Renderer::ResourceUpload PhxEngine::Renderer::RenderScene::UpdateMaterialData(RHI::IGraphicsDevice* gfxDevice, Scene::Scene& scene)
{
	auto materialEntitiesView = scene.GetAllEntitiesWith<Scene::MaterialComponent>();

	RHI::BufferDesc desc = {};
	desc.DebugName = "Material Data";
	desc.Binding = RHI::BindingFlags::ShaderResource;
	desc.InitialState = ResourceStates::ShaderResource;
	desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Structured;
	desc.CreateBindless = true;
	desc.StrideInBytes = sizeof(Shader::New::Material);
	desc.SizeInBytes = sizeof(Shader::New::Material) * materialEntitiesView.size();

	this->MaterialBuffer = IGraphicsDevice::GPtr->CreateBuffer({
			.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Structured,
			.Binding = RHI::BindingFlags::ShaderResource,
			.InitialState = RHI::ResourceStates::ShaderResource,
			.StrideInBytes = sizeof(Shader::New::Material),
			.SizeInBytes = sizeof(Shader::New::Material) * materialEntitiesView.size(),
			.CreateBindless = true,
			.DebugName = "Material Data",
		});

	ResourceUpload mtlUploader = Renderer::CreateResourceUpload(sizeof(Shader::New::Material) * materialEntitiesView.size());
	Shader::New::Material* pMaterialUploadBufferData = reinterpret_cast<Shader::New::Material*>(mtlUploader.Data);
	uint32_t currMat = 0;
	for (auto e : materialEntitiesView)
	{
		auto& mat = materialEntitiesView.get<Scene::MaterialComponent>(e);

		Shader::New::Material* shaderData = pMaterialUploadBufferData + currMat;

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

	return mtlUploader;
}

Renderer::ResourceUpload PhxEngine::Renderer::RenderScene::UpdateGeometryData(RHI::IGraphicsDevice* gfxDevice, RHI::ICommandList* commandList, Scene::Scene& scene)
{
	// Create Geometry Data
	auto meshEntitiesView = scene.GetAllEntitiesWith<Scene::MeshComponent>();
	uint32_t numGeometryData = 0;
	for (auto e : meshEntitiesView)
	{
		auto comp = meshEntitiesView.get<Scene::MeshComponent>(e);
		numGeometryData += comp.Surfaces.size();
	}

	RHI::BufferDesc desc = {};
	desc.DebugName = "Geometry Data";
	desc.Binding = RHI::BindingFlags::ShaderResource;
	desc.InitialState = ResourceStates::ShaderResource;
	desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Raw;
	desc.CreateBindless = true;
	desc.StrideInBytes = sizeof(Shader::Geometry);
	desc.SizeInBytes = sizeof(Shader::Geometry) * numGeometryData;

	this->GeometryBuffer = IGraphicsDevice::GPtr->CreateBuffer({
			.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Structured,
			.Binding = RHI::BindingFlags::ShaderResource,
			.InitialState = RHI::ResourceStates::ShaderResource,
			.StrideInBytes = sizeof(Shader::Geometry),
			.SizeInBytes = sizeof(Shader::Geometry) * numGeometryData,
			.CreateBindless = true,
			.DebugName = "Geometry Data",
		});

	ResourceUpload uploader = Renderer::CreateResourceUpload(sizeof(Shader::New::Material) * sizeof(Shader::Geometry) * numGeometryData);
	uint32_t geometryCount = 0;
	for (auto e : meshEntitiesView)
	{
		auto& mesh = meshEntitiesView.get<Scene::MeshComponent>(e);
		for (auto& surface : mesh.Surfaces)
		{
			// Calclulate Bounding data
			// Create Meshlet data
		}

	}
	return uploader;
}
