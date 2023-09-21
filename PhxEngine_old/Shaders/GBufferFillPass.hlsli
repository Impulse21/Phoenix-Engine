#ifndef __GBUFFER_FILL_PASS_HLSL__
#define __GBUFFER_FILL_PASS_HLSL__

// Helpers for Syntax Code writhing
// #define COMPILE_VS
// #define DEPTH_PASS
// #define COMPILE_PS

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures.h"

#include "Globals.hlsli"
#include "VertexBuffer.hlsli"
#include "Defines.hlsli"
#include "GBuffer.hlsli"

PUSH_CONSTANT(push, GeometryPassPushConstants);

struct PSInput
{
    float3 NormalWS : NORMAL;
    float4 Colour : COLOUR;
    float2 TexCoord : TEXCOORD;
    float3 ShadowTexCoord : TEXCOORD1;
    float3 PositionWS : Position;
    float4 TangentWS : TANGENT;
    uint MaterialID : MATERIAL;
    float4 Position : SV_POSITION;
};

#ifdef COMPILE_VS
inline ShaderMeshInstancePointer GetMeshInstancePtr(uint index)
{
    return ResourceHeap_GetBuffer(push.InstancePtrBufferDescriptorIndex).Load<ShaderMeshInstancePointer>(push.InstancePtrDataOffset + index * sizeof(ShaderMeshInstancePointer));
}

[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
PSInput main(
	uint vertexID : SV_VertexID,
	uint instanceID : SV_InstanceID)
{
    Geometry geometry = LoadGeometry(push.GeometryIndex);
    VertexData vertexData = RetrieveVertexData(vertexID, geometry);
    
    ShaderMeshInstancePointer instancePtr = GetMeshInstancePtr(instanceID);
    MeshInstance meshInstance = LoadMeshInstance(instancePtr.GetInstanceIndex());
    
    matrix worldMatrix = meshInstance.WorldMatrix;
    
    PSInput output;
    output.PositionWS = mul(vertexData.Position, worldMatrix).xyz;
    output.Position = mul(float4(output.PositionWS, 1.0f), GetCamera().ViewProjection);

    output.NormalWS = mul(vertexData.Normal, (float3x3) worldMatrix).xyz;
    output.TexCoord = vertexData.TexCoord;
    output.Colour = float4(1.0f, 1.0f, 1.0f, 1.0f);

    output.TangentWS = float4(mul(vertexData.Tangent.xyz, (float3x3) worldMatrix), vertexData.Tangent.w);

    output.MaterialID = geometry.MaterialIndex;

    return output;
}
#endif // COMPILE_VS

#ifdef COMPILE_PS

inline Surface LoadSurfaceData(in PSInput input, in MaterialData material)
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
    MaterialData material = LoadMaterial(input.MaterialID);
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