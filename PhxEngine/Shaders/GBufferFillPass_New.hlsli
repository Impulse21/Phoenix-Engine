#ifndef __GBUFFER_FILL_PASS_HLSL__
#define __GBUFFER_FILL_PASS_HLSL__

// EDITOR HELPERS
// #define COMPILE_MS
// #define COMPILE_VS
// #define COMPILE_PS

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"

#include "Globals_New.hlsli"
#include "VertexFetch.hlsli"
#include "Defines.hlsli"
#include "GBuffer_New.hlsli"

#ifdef COMPILE_MS
#include "MeshletCommon.hlsli"
#endif

PUSH_CONSTANT(push, GeometryPushConstant);

struct PSInput
{
    float3 NormalWS : NORMAL;
    float4 Colour : COLOUR;
    float2 TexCoord : TEXCOORD;
    float3 ShadowTexCoord : TEXCOORD1;
    float4 TangentWS : TANGENT;
    uint MeshletIndex : COLOR0;
    uint MaterialID : MATERIAL;
    float3 PositionWS : Position;
    float4 Position : SV_POSITION;
};

#if defined(COMPILE_VS) || defined(COMPILE_MS)
PSInput PopulatePSInput(matrix worldMatrix, Geometry geometryData, uint vertexID)
{
    VertexData vertexData = FetchVertexData(vertexID, geometryData);
    
    PSInput output;
    output.PositionWS = mul(vertexData.Position, worldMatrix).xyz;
    output.Position = mul(float4(output.PositionWS, 1.0f), GetCamera().ViewProjection);

    output.NormalWS = mul(vertexData.Normal, (float3x3) worldMatrix).xyz;
    output.TexCoord = vertexData.TexCoord;
    output.Colour = float4(1.0f, 1.0f, 1.0f, 1.0f);

    output.TangentWS = float4(mul(vertexData.Tangent.xyz, (float3x3) worldMatrix), vertexData.Tangent.w);

    output.MaterialID = geometryData.MaterialIndex;

    return output;
}
#endif // defined(COMPILE_PS) || defined(COMPILE_CS)

#ifdef COMPILE_VS

[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
PSInput main(
	uint vertexID : SV_VertexID,
	uint instanceID : SV_InstanceID)
{
    ObjectInstance objectInstance = LoadObjectInstnace(push.DrawId);
    Geometry geometryData = LoadGeometry(objectInstance.GeometryIndex);
    matrix worldMatrix = objectInstance.WorldMatrix;

    return PopulatePSInput(worldMatrix, geometryData, vertexID);
}
#endif // COMPILE_VS

#ifdef COMPILE_MS

[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
[numthreads(128, 1, 1)]
[outputtopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    in payload MeshletPayload payload,
    out indices uint3 tris[MAX_PRIMS],
    out vertices PSInput verts[MAX_VERTS])
{
    ObjectInstance objectInstance = LoadObjectInstnace(push.DrawId);
    Geometry geometryData = LoadGeometry(objectInstance.GeometryIndex);
    
    uint meshletIndex = payload.MeshletIndices[gid];
    
      // Catch any out-of-range indices (in case too many MS threadgroups were dispatched from AS)
    if (meshletIndex >= geometryData.MeshletCount)
    {
        return;
    }
    
    Meshlet m = LoadMeshlet(geometryData.MeshletOffset + gid);

    SetMeshOutputCounts(m.VertCount, m.PrimCount);

    if (gtid < m.PrimCount)
    {
        tris[gtid] = GetPrimitive(m, gtid);
    }

    if (gtid < m.VertCount)
    {
        matrix worldMatrix = objectInstance.WorldMatrix;
        uint vertexID = GetVertexIndex(m, gtid);
        verts[gtid] = PopulatePSInput(worldMatrix, geometryData, vertexID);
    }
}

#endif // COMPILE_MS

#ifdef COMPILE_PS

inline Surface LoadSurfaceData(in PSInput input, in Material material)
{
    Surface surface = DefaultSurface();
    surface.Albedo = 1.0f;
    surface.Albedo *= material.AlbedoColour;
    if (material.AlbedoTexture != InvalidDescriptorIndex)
    {
        surface.Albedo *= ResourceHeap_GetTexture2D(material.AlbedoTexture).Sample(SamplerDefault, input.TexCoord).xyz;
    }
    
    surface.Metalness = material.Metalness;
    surface.Roughness = material.Roughness;

    // -- Sample the material texture ---
    if (material.MaterialTexture != InvalidDescriptorIndex)
    {
        float4 materialSample = ResourceHeap_GetTexture2D(material.MaterialTexture).Sample(SamplerDefault, input.TexCoord);
        surface.Metalness = materialSample.b;
        surface.Roughness = materialSample.g;
    }
    
    surface.AO = material.AO;
    if (material.AOTexture != InvalidDescriptorIndex)
    {
        surface.AO = ResourceHeap_GetTexture2D(material.AOTexture).Sample(SamplerDefault, input.TexCoord).r;
    }
    
    float3 tangent = normalize(input.TangentWS.xyz);
    surface.Normal = normalize(input.NormalWS);
    float3 biTangent = cross(surface.Normal, tangent.xyz) * input.TangentWS.w;
    
    if (material.NormalTexture != InvalidDescriptorIndex)
    {
        float3x3 tbn = float3x3(tangent, biTangent, surface.Normal);
        // float3x3 tbn = float3x3(T, B, N);
        surface.Normal = ResourceHeap_GetTexture2D(material.NormalTexture).Sample(SamplerDefault, input.TexCoord).rgb * 2.0 - 1.0;
        surface.Normal = normalize(surface.Normal);
        surface.Normal = normalize(mul(surface.Normal, tbn));
    }
    
    return surface;
}


struct PSOutput
{
    float4 Channel_0    : SV_Target0;
    float4 Channel_1    : SV_Target1;
    float4 Channel_2    : SV_Target2;
    float4 Channel_3    : SV_Target3;
};

[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
PSOutput main(PSInput input)
{
    Material material = LoadMaterial(input.MaterialID);
    Surface surface = LoadSurfaceData(input, material);

    float4 channelData[NUM_GBUFFER_CHANNELS];
    EncodeGBuffer(surface, channelData);

    // -- End Material Collection ---
    PSOutput output;
    output.Channel_0 = channelData[0];
    output.Channel_1 = channelData[1];
    output.Channel_2 = channelData[2];
    output.Channel_3 = channelData[3];

    return output;
}
#endif // COMPILE_PS

#endif // __GBUFFER_FILL_PASS_HLSL__

/*


    Lighting lightingTerms;
    lightingTerms.Init();
    
    const float3 viewIncident = input.PositionWS - (float3) GetCamera().GetPosition();
    BRDFDataPerSurface brdfSurfaceData = CreatePerSurfaceBRDFData(surface, input.PositionWS, viewIncident);
    
    DirectLightContribution(GetScene(), brdfSurfaceData, surface, lightingTerms);
    
    
    float3 finalColour = ApplyLighting(lightingTerms, brdfSurfaceData, surface);
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
    return float4(finalColour, 1.0f);
*/