#ifndef __PHX_GLOBALS_HLSLI__
#define __PHX_GLOBALS_HLSLI__


#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"
#include "ResourceHeapTables.hlsli"

#define PHX_ENGINE_DEFAULT_ROOTSIGNATURE \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=20, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
	"StaticSampler(s52, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_LESS_EQUAL),"


ConstantBuffer<Frame> FrameCB : register(b0);
ConstantBuffer<Camera> CameraCB : register(b1);

SamplerState SamplerDefault : register(s50);
SamplerState SamplerLinearClamped : register(s51);
SamplerComparisonState ShadowSampler : register(s52);

inline bool IsSaturated(float a)
{
    return a == saturate(a);
}
inline bool IsSaturated(float2 a)
{
    return all(a == saturate(a));
}
inline bool IsSaturated(float3 a)
{
    return all(a == saturate(a));
}
inline bool IsSaturated(float4 a)
{
    return all(a == saturate(a));
}

inline float2 ClipSpaceToUV(in float2 clipspace)
{
	return clipspace * float2(0.5, -0.5) + 0.5;
}

inline float3 ClipSpaceToUV(in float3 clipspace)
{
	return clipspace * float3(0.5, -0.5, 0.5) + 0.5;
}

inline Frame GetFrame()
{
	return FrameCB;
}

inline Camera GetCamera()
{
    return CameraCB;
}

inline Scene GetScene()
{
    return GetFrame().SceneData;
}

inline Geometry LoadGeometry(uint index)
{
    StructuredBuffer<Geometry> buffer = ResourceDescriptorHeap[GetScene().GeometryBufferIdx];
    return buffer[index];
}

inline float4 LoadGeometryBounds(uint index)
{
    StructuredBuffer<float4> buffer = ResourceDescriptorHeap[GetScene().GeometryBoundsBufferIdx];
    return buffer[index];
}

inline Material LoadMaterial(uint index)
{
    StructuredBuffer<Material> buffer = ResourceDescriptorHeap[GetScene().MaterialBufferIdx];
    return buffer[index];
}

inline ObjectInstance LoadObjectInstnace(uint index)
{
    StructuredBuffer<ObjectInstance> buffer = ResourceDescriptorHeap[GetScene().ObjectBufferIdx];
    return buffer[index];
}

inline CullData LoadMeshletCullData(uint index)
{
    StructuredBuffer<CullData> buffer = ResourceDescriptorHeap[GetScene().MeshletCullDataBufferIdx];
    return buffer[index];
}

inline float4x4 LoadMatrix(uint matrixIndex)
{
    return 0;
}
inline Light LoadLight(uint index)
{
    StructuredBuffer<Light> buffer = ResourceDescriptorHeap[GetFrame().LightBufferIdx];
    return buffer[index];
}

inline uint GetLightBin(uint index)
{
    StructuredBuffer<uint> buffer = ResourceDescriptorHeap[GetFrame().LightLutBufferIndex];
    return buffer[index];
}

inline uint GetLightTile(uint index)
{
    StructuredBuffer<uint> buffer = ResourceDescriptorHeap[GetFrame().LightTilesBufferIndex];
    return buffer[index];
}

inline uint GetLightIndex(uint index)
{
    StructuredBuffer<uint> buffer = ResourceDescriptorHeap[GetFrame().SortedLightBufferIndex];
    return buffer[index];
}

inline matrix GetLightMatrix(uint index)
{
    StructuredBuffer<matrix> buffer = ResourceDescriptorHeap[GetFrame().LightMatrixBufferIdx];
    return buffer[index];
}

// Convert texture coordinates on a cubemap face to cubemap sampling coordinates:
// direction	: direction that is usable for cubemap sampling
// returns float3 that has uv in .xy components, and face index in Z component
//	https://stackoverflow.com/questions/53115467/how-to-implement-texturecube-using-6-sampler2d
inline float3 CubemapToUv(in float3 r)
{
    float faceIndex = 0;
    float3 absr = abs(r);
    float3 uvw = 0;
    if (absr.x > absr.y && absr.x > absr.z)
    {
		// x major
        float negx = step(r.x, 0.0);
        uvw = float3(r.zy, absr.x) * float3(lerp(-1.0, 1.0, negx), -1, 1);
        faceIndex = negx;
    }
    else if (absr.y > absr.z)
    {
		// y major
        float negy = step(r.y, 0.0);
        uvw = float3(r.xz, absr.y) * float3(1.0, lerp(1.0, -1.0, negy), 1.0);
        faceIndex = 2.0 + negy;
    }
    else
    {
		// z major
        float negz = step(r.z, 0.0);
        uvw = float3(r.xy, absr.z) * float3(lerp(1.0, -1.0, negz), -1, 1);
        faceIndex = 4.0 + negz;
    }
    return float3((uvw.xy / uvw.z + 1) * 0.5, faceIndex);
}
#endif // __PHX_GLOBALS_HLSLI__