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
	"StaticSampler(s52, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_GREATER_EQUAL),"


ConstantBuffer<Frame> FrameCB : register(b0);
ConstantBuffer<Camera> CameraCB : register(b1);

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

inline Geometry LoadGeometry(uint index)
{
    StructuredBuffer<Geometry> buffer = ResourceDescriptorHeap[GetScene().GeometryBufferIdx];
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
    StructuredBuffer<ObjectInstance> buffer = ResourceDescriptorHeap[GetScene().MeshletCullDataBufferIdx];
    return buffer[index];
}
#endif // __PHX_GLOBALS_HLSLI__