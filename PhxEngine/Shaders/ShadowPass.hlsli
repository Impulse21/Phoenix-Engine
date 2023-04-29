#ifndef __SHADOW_PASS_HLSL__
#define __SHADOW_PASS_HLSL__

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"

#include "Globals_New.hlsli"
#include "VertexFetch.hlsli"
#include "Defines.hlsli"

// #define COMPILE_MS
#ifdef COMPILE_MS
#include "MeshletCommon.hlsli"
#endif

#define ShadowPassRS \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=1, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "CBV(b2),"  \

PUSH_CONSTANT(push, GeometryPushConstant);


// TODO: Consider switching this so we bind a global buffer.
ConstantBuffer<ShadowCams> SCam : register(b2);

#ifdef COMPILE_MS

struct VertexAttributes
{
    float4 Position : SV_POSITION;
};

struct PrimitiveAttributes
{
    uint ViewportId : SV_ViewportArrayIndex;
};
    
VertexAttributes PopulateVertexAttributes(ObjectInstance objectInstance, Geometry geometryData, uint vertexID, float4x4 viewProjection)
{
    VertexData vertexData = FetchVertexData(vertexID, geometryData);
    
    float4x4 worldMatrix = objectInstance.WorldMatrix;
    
    // TODO: USE THE CAMERA FACE 
    VertexAttributes output;
    output.Position = mul(vertexData.Position, worldMatrix);
    output.Position = mul(output.Position, viewProjection);

    return output;
}

[RootSignature(ShadowPassRS)]
[numthreads(128, 1, 1)]
[outputtopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint2 gid : SV_GroupID,
    in payload MeshletShadowPayload payload,
    out indices uint3 tris[MAX_PRIMS],
    out primitives PrimitiveAttributes sharedPrimitives[MAX_PRIMS],
    out vertices VertexAttributes verts[MAX_VERTS])
{
    ObjectInstance objectInstance = LoadObjectInstnace(push.DrawId);
    Geometry geometryData = LoadGeometry(objectInstance.GeometryIndex);
    
    uint meshletIndex = payload.MeshletIndices[gid.y][gid.x];
    if (meshletIndex >= geometryData.MeshletCount)
    {
        return;
    }
    
    Meshlet m = LoadMeshlet(geometryData.MeshletOffset + meshletIndex);
    SetMeshOutputCounts(m.VertCount, m.PrimCount);
    
    if (gtid < m.PrimCount)
    {
        tris[gtid] = GetPrimitive(m, gtid + geometryData.MeshletPrimtiveOffset);
        PrimitiveAttributes primAttr;
        primAttr.ViewportId = gid.y;
        sharedPrimitives[gtid] = primAttr;

    }

    if (gtid < m.VertCount)
    {
        uint vertexID = GetVertexIndex(m, gtid + geometryData.MeshletUniqueVertexIBOffset);
        verts[gtid] = PopulateVertexAttributes(objectInstance, geometryData, vertexID, SCam.ViewProjection[gid.y]);
    }
}

#endif

#ifdef COMPILE_VS
[RootSignature(ShadowPassRS)]
void main(
	in uint inVertexID : SV_VertexID,
	in uint inInstanceID : SV_InstanceID,
	out float4 outPosition : SV_POSITION,
	out uint outViewportId : SV_ViewportArrayIndex)
{
    ObjectInstance objectInstance = LoadObjectInstnace(push.DrawId);
    Geometry geometryData = LoadGeometry(objectInstance.GeometryIndex);
	
    VertexData vertexData = FetchVertexData(inVertexID, geometryData);
    float4x4 worldMatrix = objectInstance.WorldMatrix;
	// TODO: Set Light Projections
    float4x4 lightViewProj = SCam.ViewProjection[inInstanceID];
	
    outPosition = mul(vertexData.Position, worldMatrix);
    outPosition = mul(outPosition, lightViewProj);
    outViewportId = inInstanceID;
}
#endif

#endif // __SHADOW_PASS_HLSL__