#pragma once

#include <stdint.h>
#include <DirectXMath.h>

namespace PhxEngine::Renderer
{
	struct MaterialData
	{
		DirectX::XMFLOAT3 AlbedoColour;
		RHI::DescriptorIndex AlbedoTexture;

		// -- 16 byte boundary ----
		
		RHI::DescriptorIndex MaterialTexture;
		RHI::DescriptorIndex RoughnessTexture;
		RHI::DescriptorIndex MetalnessTexture;
		RHI::DescriptorIndex AOTexture;

		// -- 16 byte boundary ----

		RHI::DescriptorIndex  NormalTexture;
		float Metalness;
		float Rougness;
		float AO;

		// -- 16 byte boundary ----
	};

	struct GeometryData
	{
		uint32_t	NumIndices;
		uint32_t	NumVertices;
		int32_t		IndexBufferIndex;
		uint32_t	IndexOffset;

		// -- 16 byte boundary ----

		int			VertexBufferIndex;
		uint32_t	PositionOffset;
		uint32_t	TexCoordOffset;
		uint32_t	NormalOffset;

		// -- 16 byte boundary ----

		uint32_t	TangentOffset;
		uint32_t	MaterialIndex;
		uint32_t	_padding;
		uint32_t	_padding1;

		// -- 16 byte boundary ----
	};

	struct MeshInstanceData
	{
		uint32_t FirstGeometryIndex;
		uint32_t NumGeometries;
		uint32_t _padding;
		uint32_t _padding1;

		// -- 16 byte boundary ----

		DirectX::XMFLOAT4X4 Transform;

		// -- 16 byte boundary ----
	};
}