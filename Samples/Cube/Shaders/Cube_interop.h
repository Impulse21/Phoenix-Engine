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

using float2	= hlslpp::interop::float2;
using float3	= hlslpp::interop::float3;
using float4	= hlslpp::interop::float4;

using uint		= uint32_t;
using uint2		= hlslpp::interop::uint2;
using uint3		= hlslpp::interop::uint3;
using uint4		= hlslpp::interop::uint4;

using int2		= hlslpp::interop::int2;
using int3		= hlslpp::interop::int3;
using int4		= hlslpp::interop::int4;

#endif

struct Vertex
{
    float3 position;
    float3 normal;
    float2 uv;
};

struct DrawData
{
    Vertex*   vertices;
    float4x4  mvp;
};