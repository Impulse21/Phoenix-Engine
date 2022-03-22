#ifndef __PHX_SHADER_INTEROP_HLSLI__
#define __PHX_SHADER_INTEROP_HLSLI__


#ifdef __cplusplus

#include <DirectXMath.h>

using float4x4 = DirectX::XMFLOAT4X4;
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

#define CONSTANT_BUFFER(name, type, slot)
#define PUSH_CONSTANT(name, type)

#else

#define CONSTANT_BUFFER(name, type) ConstantBuffer<type> name : register(b999)
#define PUSH_CONSTANT(name, type) ConstantBuffer<type> name : register(b999)

#endif

#define RESOURCE_HEAP_BUFFER_SPACE     space100
#define RESOURCE_HEAP_TEX2D_SPACE      space101
#define RESOURCE_HEAP_TEX_CUBE_SPACE   space102

#endif // __PHX_SHADER_INTEROP_HLSLI__