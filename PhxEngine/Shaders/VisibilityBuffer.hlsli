#ifndef __VISIBILITY_BUFFER_HLSL__
#define __VISIBILITY_BUFFER_HLSL__

#include "Globals.hlsli"

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures.h"

PUSH_CONSTANT(push, GeometryPassPushConstants);

#define VisibilityBufferFillRS \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=19, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \

struct PSInput_VisBuffer
{
    uint InstanceIndex : INSTANCEINDEX;
    float4 Position : SV_POSITION;
    
};

struct VisibilityID
{
    uint PrimitiveIndex;
    uint InstanceIndex;
    uint SubsetIndex;
    
    uint Pack()
    {
        MeshInstance instance = LoadMeshInstance(InstanceIndex);
        Geometry geometry = LoadGeometry(instance.GeometryOffset + SubsetIndex);
        
        uint meshletIndex = instance.MeshletOffset + geometry.MeshletOffset + PrimitiveIndex / MESHLET_TRIANGLE_COUNT;
        meshletIndex += 1;
        meshletIndex &= ~0u >> 8u; // Set the lower 24 bits to mesh index.
        
        uint meshletPrimitiveIndex = PrimitiveIndex % MESHLET_TRIANGLE_COUNT;
        meshletPrimitiveIndex &= 0xFF;
        meshletPrimitiveIndex <<= 24;
        
        return meshletPrimitiveIndex | meshletIndex;
    }
};
#endif