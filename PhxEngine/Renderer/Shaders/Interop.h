#pragma once

#ifdef __cplusplus

#define STATIC_ASSERT_SIZE_OF(T, ExpectedSize) \
   static_assert(sizeof(T) == (ExpectedSize), "Size of #T must be #ExpectedSize  bytes, but is #sizeof(T) bytes.")
#else
#define STATIC_ASSERT_SIZE_OF(T, ExpectedSize)
#endif

#ifdef __cplusplus

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

namespace phx::renderer
{
#endif

	struct VertexStreamDesc
	{
		// 5-bit stride (0-31 bytes) leaves room for 16-byte streams
		// (tangent/joints/weights, all interop::float4/uint4) -- a 4-bit
		// field previously truncated stride 16 to 0, silently corrupting
		// any mesh with those attributes.
		uint Stride5_Offset27;

#ifndef __cplusplus
#else
		inline void Set(uint stride, uint offset)
		{
			this->Stride5_Offset27 = ((stride & 0x1Fu) << 27u) | (offset & 0x07FFFFFFu);
		}

		inline uint GetStride() const
		{
			return (this->Stride5_Offset27 >> 27u) & 0x1Fu;
		}

		inline uint GetOffset() const
		{
			return this->Stride5_Offset27 & 0x07FFFFFFu;
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
		VertexStreamDesc desc[VertexStream_Count];
	};

	STATIC_ASSERT_SIZE_OF(VertexStreamsHeader, 4 * VertexStream_Count);
#ifdef __cplusplus
}
#endif