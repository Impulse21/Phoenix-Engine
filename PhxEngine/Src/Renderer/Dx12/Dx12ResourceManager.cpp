#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "Dx12ResourceManager.h"


using namespace PhxEngine;
using namespace PhxEngine::Renderer;


// -- Create ---
TextureHandle Dx12ResourceManager::CreateTexture(RHI::TextureDesc const& desc)
{
	return {};
}

BufferHandle Dx12ResourceManager::CreateBuffer(RHI::BufferDesc const& desc)
{
	return {};
}

BufferHandle Dx12ResourceManager::CreateIndexBuffer(RHI::BufferDesc const& desc)
{
	return {};
}

BufferHandle Dx12ResourceManager::CreateVertexBuffer(RHI::BufferDesc const& desc)
{
	return {};
}

MeshHandle Dx12ResourceManager::CreateMesh()
{
	return {};
}

MaterialHandle Dx12ResourceManager::CreateMaterial()
{
	return {};
}

ShaderHandle Dx12ResourceManager::CreateShader()
{
	return {};
}

LightHandle Dx12ResourceManager::CreateLight()
{
	return {};
}


// -- Delete ---
void Dx12ResourceManager::DeleteTexture(TextureHandle texture)
{

}

void Dx12ResourceManager::DeleteBuffer(BufferHandle buffer)
{

}

void Dx12ResourceManager::DeleteMesh(MeshHandle mesh)
{

}

void Dx12ResourceManager::DeleteShader(ShaderHandle shader)
{

}

void Dx12ResourceManager::DeleteLight(LightHandle light)
{

}


// -- Retireve Desc ---
RHI::TextureDesc Dx12ResourceManager::GetTextureDesc(TextureHandle texture)
{
	return {};
}

RHI::BufferDesc Dx12ResourceManager::GetBufferDesc(BufferHandle buffer)
{
	return {};
}


// -- Retrieve Descriptor Indices ---
RHI::DescriptorIndex Dx12ResourceManager::GetTextureDescriptorIndex(TextureHandle texture)
{
	return {};
}

RHI::BufferDesc Dx12ResourceManager::GetBufferDescriptorIndex(BufferHandle buffer)
{
	return {};
}

// -- Material Data ---
void Dx12ResourceManager::SetAlbedo(DirectX::XMFLOAT4 const& albedo)
{
}

DirectX::XMFLOAT4 Dx12ResourceManager::GetAlbedo()
{
	return {};
}

void Dx12ResourceManager::SetEmissive(DirectX::XMFLOAT4 const& emissive)
{
}

DirectX::XMFLOAT4 Dx12ResourceManager::GetEmissive()
{
	return {};
}

void Dx12ResourceManager::SetMetalness(float metalness)
{
}

float Dx12ResourceManager::GetMetalness()
{
	return {};
}

void Dx12ResourceManager::SetRoughness(float roughness)
{
}

float Dx12ResourceManager::GetRoughtness()
{
	return {};
}

void Dx12ResourceManager::SetAO(float ao)
{
}

float Dx12ResourceManager::GetAO()
{
	return {};
}

void Dx12ResourceManager::SetDoubleSided(bool isDoubleSided)
{
}

bool Dx12ResourceManager::IsDoubleSided()
{
	return {};
}

void Dx12ResourceManager::SetAlbedoTexture(TextureHandle texture)
{
}

TextureHandle Dx12ResourceManager::GetAlbedoTexture()
{
	return {};
}

void Dx12ResourceManager::SetMaterialTexture(TextureHandle texture)
{
}

TextureHandle Dx12ResourceManager::GetMaterialTexture()
{
	return {};
}

void Dx12ResourceManager::SetAoTexture(TextureHandle texture)
{
}

TextureHandle Dx12ResourceManager::GetAoTexture()
{
	return {};
}

void Dx12ResourceManager::SetNormalMapTexture(TextureHandle texture)
{
}

TextureHandle Dx12ResourceManager::GetNormalMapTexture()
{
	return {};
}


void Dx12ResourceManager::SetBlendMode(BlendMode blendMode)
{

}

BlendMode Dx12ResourceManager::GetBlendMode()
{
	return BlendMode::Opaque;
}
// -- Material Data END---
