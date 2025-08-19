#pragma once

#ifdef __cplusplus
#define STATIC_ASSERT_SIZE_OF(T, ExpectedSize) \
   static_assert(sizeof(T) == (ExpectedSize), "Size of #T must be #ExpectedSize  bytes, but is #sizeof(T) bytes.")
#else
#define STATIC_ASSERT_SIZE_OF(T, ExpectedSize)
#endif
#ifdef __cplusplus

#if false
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
#else
#include <hlsl++.h>

using float4x4	= hlslpp::float4x4;
using float3x3	= hlslpp::float3x3;
using float2	= hlslpp::float2;
using float3	= hlslpp::float3;
using float4	= hlslpp::float4;

using uint		= uint32_t;
using uint2		= hlslpp::uint2;
using uint3		= hlslpp::uint3;
using uint4		= hlslpp::uint4;

using int2		= hlslpp::int2;
using int3		= hlslpp::int3;
using int4		= hlslpp::int4;
#endif
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

	enum VertexStreamType
	{
		VertexStream_Position = 0,
		VertexStream_Tangent,
		VertexStream_Normal,
		VertexStream_Texcoord0,
		VertexStream_Texcoord1,
		VertexStream_Colour0,
		VertexStream_Joints0,
		VertexStream_Weights0,
		VertexStream_Count,

	};

	struct VertexStreamsHeader
	{
		VertexStreamDesc Desc[VertexStream_Count];
	};

	STATIC_ASSERT_SIZE_OF(VertexStreamsHeader, 4 * VertexStream_Count);
#ifdef __cplusplus
}
#endif