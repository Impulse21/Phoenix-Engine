#ifndef __PHX_GLOBALS_HLSLI__
#define __PHX_GLOBALS_HLSLI__


#include "ShaderInterop.h"
#include "ShaderInteropStructures.h"
#include "ResourceHeapTables.hlsli"

#define PHX_ENGINE_DEFAULT_ROOTSIGNATURE \
	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"RootConstants(num32BitConstants=12, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
	"DescriptorTable( " \
		"SRV(t0, space = 100, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 101, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 102, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
	"), " \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
	"StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
	"StaticSampler(s52, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_LESS_EQUAL),"




ConstantBuffer<Frame> FrameCB : register(b0);
ConstantBuffer<Camera> CameraCB : register(b1);
StructuredBuffer<ShaderLight> LightSB : register(t0);

// StructuredBuffer<matrix> InstanceTransformsSB : register(t0);

SamplerState SamplerDefault : register(s50);
// SamplerState SamplerBrdf : register(s51);
SamplerComparisonState ShadowSampler : register(s52);


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
#endif // __PHX_GLOBALS_HLSLI__