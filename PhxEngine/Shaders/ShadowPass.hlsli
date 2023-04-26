#ifndef __SHADOW_PASS_HLSL__
#define __SHADOW_PASS_HLSL__

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"

#include "Globals_New.hlsli"
#include "VertexFetch.hlsli"
#include "Defines.hlsli"

#define COMPILE_MS
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


struct PSInput
{
    float4 Position : SV_ViewportArrayIndex;
};
    

struct PrimitiveOutput
{
    uint viewportId : SV_ViewportArrayIndex;
};
    
PSInput PopulatePSInput(ObjectInstance objectInstance, Geometry geometryData, uint vertexID)
{
    VertexData vertexData = FetchVertexData(vertexID, geometryData);
    
    float4x4 worldMatrix = objectInstance.WorldMatrix;
    
    // TODO: USE THE CAMERA FACE 
    PSInput output;
    output.Position = mul(vertexData.Position, worldMatrix).xyz;
    output.Position = mul(float4(output.Position.xyz, 1.0f), GetCamera().ViewProjection);

    return output;
}

[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
[numthreads(128, 1, 1)]
[outputtopology("triangle")]
void main(
    uint2 gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    in payload MeshletPayload payload,
    out indices uint3 tris[MAX_PRIMS],
    out primitives PrimitiveOutput prims[MAX_PRIMS],
    out vertices PSInput verts[MAX_VERTS])
{
    ObjectInstance objectInstance = LoadObjectInstnace(push.DrawId);
    Geometry geometryData = LoadGeometry(objectInstance.GeometryIndex);
    
    uint meshletIndex = payload.MeshletIndices[gid.x];
    
      // Catch any out-of-range indices (in case too many MS threadgroups were dispatched from AS)
    if (meshletIndex >= geometryData.MeshletCount)
    {
        return;
    }
    
    Meshlet m = LoadMeshlet(geometryData.MeshletOffset + meshletIndex);

    SetMeshOutputCounts(m.VertCount, m.PrimCount);

    // TODO: I AM HERE SET THE VEIEWPORT PASED ON Y OR Z THREAD ID.
    // TODO: Check if I am using the thread ID right.
    if (gtid.x < m.PrimCount)
    {
        tris[gtid.x] = GetPrimitive(m, gtid.x + geometryData.MeshletPrimtiveOffset);
        PrimitiveOutput primOutput;
        primOutput.viewportId = gtid.y;
        prims[gtid.x] = primOutput;

    }

    if (gtid < m.VertCount)
    {
        uint vertexID = GetVertexIndex(m, gtid.x + geometryData.MeshletUniqueVertexIBOffset);
        verts[gtid.x] = PopulatePSInput(objectInstance, geometryData, vertexID);
    }
}

#endif

#define COMPILE_VS
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