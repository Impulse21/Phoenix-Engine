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

namespace DefaultRootParameters
{
	enum
	{
		FrameCB = 0,
		CameraCB,
		DEP_Light,
	};
}

#define CONSTANT_BUFFER(name, type, slot)
#define PUSH_CONSTANT(name, type)
#define RS_PUSH_CONSTANT
#else

#define CONSTANT_BUFFER(name, type) ConstantBuffer<type> name : register(b999)
#define PUSH_CONSTANT(name, type) ConstantBuffer<type> name : register(b999)
#define RS_PUSH_CONSTANT "CBV(b999, space = 1, flags = DATA_STATIC)"

#endif

#define DEFERRED_BLOCK_SIZE_X 16
#define DEFERRED_BLOCK_SIZE_Y 16


#define RESOURCE_HEAP_BUFFER_SPACE			space100
#define RESOURCE_HEAP_TEX2D_SPACE			space101
#define RESOURCE_HEAP_TEX_CUBE_SPACE		space102
#define RESOURCE_HEAP_TEX_CUBE_ARRAY_SPACE	space103
#define RESOURCE_HEAP_RWTEX2DARRAY_SPACE	space104
#define RESOURCE_HEAP_RT_ACCEL_STRUCTURE    space110

#endif // __PHX_SHADER_INTEROP_HLSLI__