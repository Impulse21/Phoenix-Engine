#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Scene/Assets.h>

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Core/Helpers.h>

using namespace DirectX;
using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Scene;
using namespace PhxEngine::Scene::Assets;
using namespace PhxEngine::Renderer;

Assets::Mesh::Mesh()
	: Assets::Mesh(std::string())
{
}

Assets::Mesh::Mesh(std::string const& meshName)
{
	this->CreateRenderResourceIfEmpty();
}

Assets::Mesh::~Mesh()
{
	RHI::IGraphicsDevice::Ptr->DeleteBuffer(this->IndexGpuBuffer);
	RHI::IGraphicsDevice::Ptr->DeleteBuffer(this->VertexGpuBuffer);
	RHI::IGraphicsDevice::Ptr->DeleteRtAccelerationStructure(this->Blas);
}

void Assets::Mesh::ReverseWinding()
{
	assert(this->IsTriMesh());
	for (auto iter = this->Indices.begin(); iter != this->Indices.end(); iter += 3)
	{
		std::swap(*iter, *(iter + 2));
	}
}

void Assets::Mesh::ComputeTangentSpace()
{
	// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/#tangent-and-bitangent

	assert(this->IsTriMesh());
	assert(!this->VertexPositions.empty());

	const size_t vertexCount = this->VertexPositions.size();
	this->VertexTangents.resize(vertexCount);

	std::vector<XMVECTOR> tangents(vertexCount);
	std::vector<XMVECTOR> bitangents(vertexCount);

	for (int i = 0; i < this->Indices.size(); i += 3)
	{
		auto& index0 = this->Indices[i + 0];
		auto& index1 = this->Indices[i + 1];
		auto& index2 = this->Indices[i + 2];

		// Vertices
		XMVECTOR pos0 = XMLoadFloat3(&this->VertexPositions[index0]);
		XMVECTOR pos1 = XMLoadFloat3(&this->VertexPositions[index1]);
		XMVECTOR pos2 = XMLoadFloat3(&this->VertexPositions[index2]);

		// UVs
		XMVECTOR uvs0 = XMLoadFloat2(&this->VertexTexCoords[index0]);
		XMVECTOR uvs1 = XMLoadFloat2(&this->VertexTexCoords[index1]);
		XMVECTOR uvs2 = XMLoadFloat2(&this->VertexTexCoords[index2]);

		XMVECTOR deltaPos1 = pos1 - pos0;
		XMVECTOR deltaPos2 = pos2 - pos0;

		XMVECTOR deltaUV1 = uvs1 - uvs0;
		XMVECTOR deltaUV2 = uvs2 - uvs0;

		// TODO: Take advantage of SIMD better here
		float r = 1.0f / (XMVectorGetX(deltaUV1) * XMVectorGetY(deltaUV2) - XMVectorGetY(deltaUV1) * XMVectorGetX(deltaUV2));

		XMVECTOR tangent = (deltaPos1 * XMVectorGetY(deltaUV2) - deltaPos2 * XMVectorGetY(deltaUV1)) * r;
		XMVECTOR bitangent = (deltaPos2 * XMVectorGetX(deltaUV1) - deltaPos1 * XMVectorGetX(deltaUV2)) * r;

		tangents[index0] += tangent;
		tangents[index1] += tangent;
		tangents[index2] += tangent;

		bitangents[index0] += bitangent;
		bitangents[index1] += bitangent;
		bitangents[index2] += bitangent;
	}

	assert(this->VertexNormals.size() == vertexCount);
	for (int i = 0; i < vertexCount; i++)
	{
		const XMVECTOR normal = XMLoadFloat3(&this->VertexNormals[i]);
		const XMVECTOR& tangent = tangents[i];
		const XMVECTOR& bitangent = bitangents[i];

		// Gram-Schmidt orthogonalize

		DirectX::XMVECTOR orthTangent = DirectX::XMVector3Normalize(tangent - normal * DirectX::XMVector3Dot(normal, tangent));
		float sign = DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMVector3Cross(normal, tangent), bitangent)) > 0
			? -1.0f
			: 1.0f;

		XMVectorSetW(tangent, sign);
		DirectX::XMStoreFloat4(&this->VertexTangents[i], orthTangent);
	}
}

void Assets::Mesh::CreateRenderData(RHI::CommandListHandle commandList)
{
	// Construct the Mesh buffer
	if (!this->Indices.empty() && !this->IndexGpuBuffer.IsValid())
	{
		RHI::BufferDesc indexBufferDesc = {};
		indexBufferDesc.SizeInBytes = sizeof(uint32_t) * this->Indices.size();
		indexBufferDesc.StrideInBytes = sizeof(uint32_t);
		indexBufferDesc.DebugName = "Index Buffer";
		this->IndexGpuBuffer = RHI::IGraphicsDevice::Ptr->CreateIndexBuffer(indexBufferDesc);

		commandList->TransitionBarrier(this->IndexGpuBuffer, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
		commandList->WriteBuffer<uint32_t>(this->IndexGpuBuffer, this->Indices);
		commandList->TransitionBarrier(
			this->IndexGpuBuffer,
			RHI::ResourceStates::CopyDest,
			RHI::ResourceStates::IndexGpuBuffer | RHI::ResourceStates::ShaderResource);
	}

	// Construct the Vertex Buffer
	// Set up the strides and offsets
	RHI::BufferDesc vertexDesc = {};
	if (!this->VertexGpuBuffer.IsValid())
	{
		RHI::BufferDesc vertexDesc = {};
		vertexDesc.StrideInBytes = sizeof(float);
		vertexDesc.DebugName = "Vertex Buffer";
		vertexDesc.EnableBindless();
		vertexDesc.IsRawBuffer();
		vertexDesc.Binding = RHI::BindingFlags::VertexBuffer | RHI::BindingFlags::ShaderResource;

		const uint64_t alignment = 16ull;
		vertexDesc.SizeInBytes =
			Helpers::AlignTo(this->VertexPositions.size() * sizeof(DirectX::XMFLOAT3), alignment) +
			Helpers::AlignTo(this->VertexTexCoords.size() * sizeof(DirectX::XMFLOAT2), alignment) +
			Helpers::AlignTo(this->VertexNormals.size() * sizeof(DirectX::XMFLOAT3), alignment) +
			Helpers::AlignTo(this->VertexTangents.size() * sizeof(DirectX::XMFLOAT4), alignment) +
			Helpers::AlignTo(this->VertexColour.size() * sizeof(DirectX::XMFLOAT3), alignment);

		// Is this Needed for Raw Buffer Type
		this->VertexGpuBuffer = RHI::IGraphicsDevice::Ptr->CreateIndexBuffer(vertexDesc);

		std::vector<uint8_t> gpuBufferData(vertexDesc.SizeInBytes);
		std::memset(gpuBufferData.data(), 0, vertexDesc.SizeInBytes);
		uint64_t bufferOffset = 0ull;

		auto WriteDataToGpuBuffer = [&](VertexAttribute attr, void* data, uint64_t sizeInBytes)
		{
			auto& bufferRange = this->GetVertexAttribute(attr);
			bufferRange.ByteOffset = bufferOffset;
			bufferRange.SizeInBytes = sizeInBytes;

			bufferOffset += Helpers::AlignTo(bufferRange.SizeInBytes, alignment);
			// DirectX::XMFLOAT3* vertices = reinterpret_cast<DirectX::XMFLOAT3*>(gpuBufferData.data() + bufferOffset);
			std::memcpy(gpuBufferData.data() + bufferRange.ByteOffset, data, bufferRange.SizeInBytes);
		};

		if (!this->VertexPositions.empty())
		{
			WriteDataToGpuBuffer(
				VertexAttribute::Position,
				this->VertexPositions.data(),
				this->VertexPositions.size() * sizeof(DirectX::XMFLOAT3));
		}

		if (!this->VertexTexCoords.empty())
		{
			WriteDataToGpuBuffer(
				VertexAttribute::TexCoord,
				this->VertexTexCoords.data(),
				this->VertexTexCoords.size() * sizeof(DirectX::XMFLOAT2));
		}

		if (!this->VertexNormals.empty())
		{
			WriteDataToGpuBuffer(
				VertexAttribute::Normal,
				this->VertexNormals.data(),
				this->VertexNormals.size() * sizeof(DirectX::XMFLOAT3));
		}

		if (!this->VertexTangents.empty())
		{
			WriteDataToGpuBuffer(
				VertexAttribute::Tangent,
				this->VertexTangents.data(),
				this->VertexTangents.size() * sizeof(DirectX::XMFLOAT4));
		}

		if (!this->VertexColour.empty())
		{
			WriteDataToGpuBuffer(
				VertexAttribute::Colour,
				this->VertexColour.data(),
				this->VertexColour.size() * sizeof(DirectX::XMFLOAT3));
		}

		// Write Data
		commandList->TransitionBarrier(this->VertexGpuBuffer, RHI::ResourceStates::Common, RHI::ResourceStates::CopyDest);
		commandList->WriteBuffer(this->VertexGpuBuffer, gpuBufferData, 0);
		commandList->TransitionBarrier(this->VertexGpuBuffer, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource);
	}

	// Create RT BLAS object
	if (RHI::IGraphicsDevice::Ptr->CheckCapability(RHI::DeviceCapability::RayTracing))
	{
		this->BlasState = BLASState::Rebuild;

		// Create Blast
		RHI::RTAccelerationStructureDesc rtDesc = {};
		rtDesc.Type = RHI::RTAccelerationStructureDesc::Type::BottomLevel;
		rtDesc.Flags == RHI::RTAccelerationStructureDesc::kPreferFastTrace;

		for (int i = 0; i < this->Surfaces.size(); i++)
		{
			auto& surface = this->Surfaces[i];

			auto& geometry = rtDesc.ButtomLevel.Geometries.emplace_back();
			geometry.Type = RHI::RTAccelerationStructureDesc::BottomLevelDesc::Geometry::Type::Triangles;
			geometry.Triangles.VertexBuffer = this->VertexGpuBuffer;
			geometry.Triangles.VertexStride = sizeof(DirectX::XMFLOAT3);
			geometry.Triangles.VertexByteOffset = 0;
			geometry.Triangles.VertexCount = (uint32_t)this->VertexPositions.size();
			geometry.Triangles.VertexFormat = RHI::FormatType::RGB32_FLOAT;

			geometry.Triangles.IndexBuffer = this->IndexGpuBuffer;
			geometry.Triangles.IndexCount = surface.NumIndices;
			geometry.Triangles.IndexOffset = surface.IndexOffsetInMesh;
		}

		this->Blas = RHI::IGraphicsDevice::Ptr->CreateRTAccelerationStructure(rtDesc);
	}
}

StandardMaterial::StandardMaterial()
{
	this->CreateRenderResourceIfEmpty();
}
#if false
void StandardMaterial::SetAlbedo(DirectX::XMFLOAT4 const& albedo)
{

}

DirectX::XMFLOAT4 StandardMaterial::GetAlbedo()
{

}

void StandardMaterial::SetEmissive(DirectX::XMFLOAT4 const& emissive)
{

}

DirectX::XMFLOAT4 StandardMaterial::GetEmissive()
{

}

void StandardMaterial::SetMetalness(float metalness)
{

}

float StandardMaterial::GetMetalness()
{

}

void StandardMaterial::SetRoughness(float roughness)
{

}

float StandardMaterial::GetRoughtness()
{

}

void StandardMaterial::SetAO(float ao)
{

}

float StandardMaterial::GetAO()
{

}

void StandardMaterial::SetDoubleSided(bool isDoubleSided)
{

}

bool IsDoubleSided()
{

}

void StandardMaterial::SetBlendMode(BlendMode blendMode)
{

}

BlendMode StandardMaterial::GetBlendMode()
{
	return BlendMode::Opaque;
}

void StandardMaterial::SetAlbedoTexture(std::shared_ptr<Texture>& texture)
{
	this->m_albedoTex = texture;
}

std::shared_ptr<Assets::Texture> StandardMaterial::GetAlbedoTexture()
{
	return this->m_albedoTex;
}


void StandardMaterial::SetMaterialTexture(std::shared_ptr<Texture>& texture)
{
	this->m_materialTex = texture;
}

std::shared_ptr<Assets::Texture> StandardMaterial::GetMaterialTexture()
{
	return this->m_materialTex;
}


void StandardMaterial::SetAoTexture(std::shared_ptr<Texture>& texture)
{
	this->m_aoTex = texture;
}

std::shared_ptr<Assets::Texture> StandardMaterial::GetAoTexture()
{
	return this->m_aoTex;
}


void StandardMaterial::SetNormalMapTexture(std::shared_ptr<Texture>& texture)
{
	this->m_normalMapTex = texture;
}

std::shared_ptr<Assets::Texture> StandardMaterial::GetNormalMapTexture()
{
	return this->m_normalMapTex;
}
#endif
void StandardMaterial::CreateRenderResourceIfEmpty()
{
}

Assets::Texture::Texture()
{
	this->CreateRenderResourceIfEmpty();
}

Assets::Texture::~Texture()
{
	RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_renderTexture);
}

void Assets::Texture::CreateRenderResourceIfEmpty()
{
	// TODO Clean up
}