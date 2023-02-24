#ifndef __VISIBILITY_FILL_PASS_RESOURCES_HLSL__
#define __VISIBILITY_FILL_PASS_RESOURCES_HLSL__

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures.h"
#include "ResourceHeapTables.hlsli"

#define VisibilityBufferFillRS \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=1, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \


PUSH_CONSTANT(push, VisibilityFillPushConstant);

ConstantBuffer<Frame_NEW> FrameCB : register(b0);
ConstantBuffer<Camera> CameraCB : register(b1);


struct PSInputVisBuffer
{
    uint DrawId : INSTANCEINDEX;
    float4 Position : SV_POSITION;
};

inline Frame_NEW GetFrame()
{
    return FrameCB;
}

inline Scene_NEW GetScene()
{
    return GetFrame().SceneData;
}

inline Camera GetCamera()
{
    return CameraCB;
}

inline Geometry LoadGeometry(uint geoIndex)
{
    StructuredBuffer<Geometry> geometryBuffer = ResourceDescriptorHeap[GetScene().GeometryBufferIdx];
    return geometryBuffer[geoIndex];
}

inline DrawPacket LoadDrawPacket(uint index)
{
    StructuredBuffer<DrawPacket> buffer = ResourceDescriptorHeap[GetScene().DrawPacketBufferIdx];
    return buffer[index];
}

inline MeshInstance LoadMeshInstance(uint index)
{
    StructuredBuffer<MeshInstance> objectBuffer = ResourceDescriptorHeap[GetScene().ObjectBufferIdx];
    return objectBuffer[index];
}
#endif
