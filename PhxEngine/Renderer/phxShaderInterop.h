#pragma once

#ifdef __cplusplus
#define STATIC_ASSERT_SIZE_OF(T, ExpectedSize) \
   static_assert(sizeof(T) == (ExpectedSize), "Size of #T must be #ExpectedSize  bytes, but is #sizeof(T) bytes.")
#else
#define STATIC_ASSERT_SIZE_OF(T, ExpectedSize)
#endif
#ifdef __cplusplus

#include <DirectXMath.h>

using float4x4 = DirectX::XMFLOAT4X4;
using float3x3 = DirectX::XMFLOAT3X3;
using float2 = DirectX::XMFLOAT2;
using float3 = DirectX::XMFLOAT3;
using float4 = DirectX::XMFLOAT4;

using uint = uint32_t;
using uint2 = DirectX::XMUINT2;
using uint3 = DirectX::XMUINT3;
using uint4 = DirectX::XMUINT4;

using int2 = DirectX::XMINT2;
using int3 = DirectX::XMINT3;
using int4 = DirectX::XMINT4;
namespace phx::renderer
{
#endif

	struct VertexStreamDesc
	{
		uint Stride4_Offset28;
		
#ifndef __cplusplus
#else
		inline void SetStride(uint stride)
		{
			this->Stride4_Offset28 |= (stride & 0xF) << 28u;
		}

		inline void SetOffset(uint offset)
		{
			this->Stride4_Offset28 |= offset & 0x0FFFFFFF;
		}
#endif
	};
	STATIC_ASSERT_SIZE_OF(VertexStreamDesc, 4);

	enum VertexStreamTypes
	{
		kPosition = 0,
		kTangents,
		kNormals,
		kUV0,
		kUV1,
		kColour,
		kJoints,
		kWeights,
		kNumStreams,

	};

	struct VertexStreamsHeader
	{
		VertexStreamDesc Desc[kNumStreams];
	};

	STATIC_ASSERT_SIZE_OF(VertexStreamsHeader, 4 * kNumStreams);
#ifdef __cplusplus
}
#endif