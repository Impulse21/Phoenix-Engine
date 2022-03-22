#ifndef __PHX_GLOBALS_HLSLI__
#define __PHX_GLOBALS_HLSLI__


#include "../Include/Shaders/ShaderInterop.h"
#include "../Include/Shaders/ShaderInteropStructures.h"
#include "Defines.hlsli"
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

// StructuredBuffer<matrix> InstanceTransformsSB : register(t0);

SamplerState SamplerDefault : register(s50);
SamplerState SamplerBrdf : register(s51);
SamplerComparisonState ShadowSampler : register(s52);

inline SceneData GetScene()
{
    return FrameCB.Scene;
}

inline Camera GetCamera()
{
    return CameraCB;
}

inline Mesh LoadMesh(uint meshIndex)
{
    return ResourceHeap_Buffer[GetScene().MeshBufferIndex].Load<Mesh>(meshIndex * sizeof(Mesh));
}

inline Geometry LoadGeometry(uint geoIndex)
{
    return ResourceHeap_Buffer[GetScene().GeometryBufferIndex].Load<Geometry>(geoIndex * sizeof(Geometry));
}

inline MaterialData LoadMaterial(uint mtlIndex)
{
    return ResourceHeap_Buffer[GetScene().MaterialBufferIndex].Load<MaterialData> (mtlIndex * sizeof(MaterialData));
}
#endif // __PHX_GLOBALS_HLSLI__