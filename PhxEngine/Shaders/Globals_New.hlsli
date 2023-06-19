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

inline uint3 DDGI_GetProbeCoord(uint probeIndex)
{
    uint3 gridDimensions = GetScene().DDGI.GridDimensions;
    
    // To 3D: https://stackoverflow.com/questions/7367770/how-to-flatten-or-index-3d-array-in-1d-array
    const uint z = probeIndex / (gridDimensions.x * gridDimensions.y);
    probeIndex -= (z * gridDimensions.x * gridDimensions.y);
    const uint y = probeIndex / gridDimensions.x;
    const uint x = probeIndex % gridDimensions.x;

    return uint3(x, y, z);
}
inline uint DDGI_GetProbeIndex(uint3 probeCoord)
{
    uint3 dimensions = GetScene().DDGI.GridDimensions;
    return (probeCoord.z * dimensions.x * dimensions.y) + (probeCoord.y * dimensions.x) + probeCoord.x;
}

inline float3 DDGI_ProbeCoordToPosition(uint3 probeCoord)
{
    float3 pos = GetScene().DDGI.GridStartPosition + probeCoord * GetScene().DDGI.CellSize;
    if (GetScene().DDGI.OffsetBufferId >= 0)
    {
        pos += ResourceHeap_GetBuffer(GetScene().DDGI.OffsetBufferId).Load<DDGIProbeOffset> (DDGI_GetProbeIndex(probeCoord) * sizeof(DDGIProbeOffset)).load();
    }
    // Add offset adjustment
    return pos;
}

// https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#rayquery-committedtrianglebarycentrics
// w is computed as 1 - u - w
// p0 * w + p1 * u + p2 * v
inline float BarycentricInterpolation(in float a0, in float a1, in float a2, float2 bary)
{
    return mad(a0, 1 - bary.x - bary.y, mad(a1, bary.x, a2 * bary.y));
}

inline float2 BarycentricInterpolation(in float2 a0, in float2 a1, in float2 a2, float2 bary)
{
    return mad(a0, 1 - bary.x - bary.y, mad(a1, bary.x, a2 * bary.y));
}
inline float3 BarycentricInterpolation(in float3 a0, in float3 a1, in float3 a2, float2 bary)
{
    return mad(a0, 1 - bary.x - bary.y, mad(a1, bary.x, a2 * bary.y));
}

inline float4 BarycentricInterpolation(in float4 a0, in float4 a1, in float4 a2, float2 bary)
{
    return mad(a0, 1 - bary.x - bary.y, mad(a1, bary.x, a2 * bary.y));
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



// Returns +/-1
float2 SignNotZero(float2 v)
{
    return float2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
}

float3 DecodeOct(float2 e)
{
    float3 v = float3(e.xy, 1.0 - abs(e.x) - abs(e.y));
    if (v.z < 0)
        v.xy = (1.0 - abs(v.yx)) * SignNotZero(v.xy);
	
    return normalize(v);

}
// Assume normalized input. Output is on [-1, 1] for each component.
float2 EncodeOct(in float3 v)
{
	// Project the sphere onto the octahedron, and then onto the xy plane
    float2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + abs(v.z)));
	// Reflect the folds of the lower hemisphere over the diagonals
    return (v.z <= 0.0) ? ((1.0 - abs(p.yx)) * SignNotZero(p)) : p;
}

#endif // __PHX_GLOBALS_HLSLI__