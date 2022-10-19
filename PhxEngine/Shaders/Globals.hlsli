#ifndef __PHX_GLOBALS_HLSLI__
#define __PHX_GLOBALS_HLSLI__


#include "Include/Shaders/ShaderInterop.h"
#include "Include/Shaders/ShaderInteropStructures.h"
#include "ResourceHeapTables.hlsli"
#if false
#define PHX_ENGINE_DEFAULT_ROOTSIGNATURE \
	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"RootConstants(num32BitConstants=12, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "SRV(t0),"  \ // TODO: Remove
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
	"StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
	"StaticSampler(s52, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_GREATER_EQUAL),"

#endif 
#define PHX_ENGINE_DEFAULT_ROOTSIGNATURE \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "SRV(t0),"  \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
	"StaticSampler(s52, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_GREATER_EQUAL),"


ConstantBuffer<Frame> FrameCB : register(b0);
ConstantBuffer<Camera> CameraCB : register(b1);
StructuredBuffer<ShaderLight> LightSB : register(t0);

// StructuredBuffer<matrix> InstanceTransformsSB : register(t0);

SamplerState SamplerDefault : register(s50);
SamplerState SamplerLinearClamped : register(s51);
SamplerComparisonState ShadowSampler : register(s52);

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

inline Atmosphere GetAtmosphere()
{
	return GetScene().AtmosphereData;
}

// -- Atmosphere helpers ---
inline float3 GetHorizonColour() { return GetAtmosphere().HorizonColour; }
inline float3 GetZenithColour() { return GetAtmosphere().ZenithColour; }

inline Mesh LoadMesh(uint meshIndex)
{
    return ResourceHeap_GetBuffer(GetScene().MeshBufferIndex).Load<Mesh>(meshIndex * sizeof(Mesh));
}

inline Geometry LoadGeometry(uint geoIndex)
{
    return ResourceHeap_GetBuffer(GetScene().GeometryBufferIndex).Load<Geometry>(geoIndex * sizeof(Geometry));
}

inline MaterialData LoadMaterial(uint mtlIndex)
{
    return ResourceHeap_GetBuffer(GetScene().MaterialBufferIndex).Load<MaterialData> (mtlIndex * sizeof(MaterialData));
}

inline ShaderLight LoadLight(uint lightIndex)
{
	return ResourceHeap_GetBuffer(GetScene().LightEntityIndex).Load<ShaderLight>(lightIndex * sizeof(ShaderLight));
}

inline float4x4 LoadMatrix(uint matrixIndex)
{
	return ResourceHeap_GetBuffer(GetScene().MatricesIndex).Load<float4x4>(matrixIndex * sizeof(float4x4));
}
#endif // __PHX_GLOBALS_HLSLI__