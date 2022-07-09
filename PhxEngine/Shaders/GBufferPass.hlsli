#ifndef __PHX_GBUFFER_PASS_HLSLI__
#define __PHX_GBUFFER_PASS_HLSLI__

#include "Globals.hlsli"
#include "Defines.hlsli"

#if USE_RESOURCE_HEAP
#define GBufferPassRS \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=19, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
	RS_BINDLESS_DESCRIPTOR_TABLE \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
	"StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
	"StaticSampler(s52, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_LESS_EQUAL),"

#else
#define GBufferPassRS \
	"RootConstants(num32BitConstants=19, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
	RS_BINDLESS_DESCRIPTOR_TABLE \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
	"StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
	"StaticSampler(s52, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_LESS_EQUAL),"

#endif 

PUSH_CONSTANT(push, GeometryPassPushConstants);

inline Mesh GetMesh()
{
    return LoadMesh(push.MeshIndex);
}

inline Geometry GetGeometry()
{
    return LoadGeometry(push.GeometryIndex);
}

inline MaterialData GetMaterial(uint materialID)
{
    return LoadMaterial(materialID);
}

struct VertexInput
{
    uint VertexID : SV_VertexID;
    uint InstanceID : SV_InstanceID;
};

    
struct PSInput
{
    float3 NormalWS     : NORMAL;
    float4 Colour       : COLOUR;
    float2 TexCoord : TEXCOORD;
    float3 ShadowTexCoord : TEXCOORD1;
    float3 PositionWS   : Position;
    float4 TangentWS    : TANGENT;
    uint MaterialID : MATERIAL;
    float4 Position     : SV_POSITION;
};

#ifdef GBUFFER_PASS_LAYOUT_COMMON
#define USE_NORMAL
#define USE_TEX_COORD
#define USE_COLOUR
#define USE_TANGENT
#define USE_MATERIAL
#endif

#ifdef GBUFFER_PASS_COMPILE_VS
[RootSignature(GBufferPassRS)]
PSInput main(in VertexInput input)
{
    PSInput output;

    Geometry geometry = LoadGeometry(push.GeometryIndex);
    ByteAddressBuffer vertexBuffer = ResourceHeap_GetBuffer(geometry.VertexBufferIndex);

    uint index = input.VertexID;

    matrix worldMatrix = push.WorldTransform;
    float4 position = float4(asfloat(vertexBuffer.Load3(geometry.PositionOffset + index * 12)), 1.0f);

    output.PositionWS = mul(position, worldMatrix).xyz;
    output.Position = mul(float4(output.PositionWS, 1.0f), GetCamera().ViewProjection);

    output.NormalWS = geometry.NormalOffset == ~0u ? 0 : asfloat(vertexBuffer.Load3(geometry.NormalOffset + index * 12));
    output.TexCoord = geometry.TexCoordOffset == ~0u ? 0 : asfloat(vertexBuffer.Load2(geometry.TexCoordOffset + index * 8));
    output.Colour = float4(1.0f, 1.0f, 1.0f, 1.0f);

    output.TangentWS = geometry.TangentOffset == ~0u ? 0 : asfloat(vertexBuffer.Load4(geometry.TangentOffset + index * 16));
    output.TangentWS = float4(mul(output.TangentWS.xyz, (float3x3) worldMatrix), output.TangentWS.w);

    output.MaterialID = geometry.MaterialIndex;

    return output;
}

#endif

#ifdef GBUFFER_PASS_COMPILE_PS

struct PSOutput
{
    float4 Channel_0    : SV_Target0;
    float4 Channel_1    : SV_Target1;
    float4 Channel_2    : SV_Target2;

    // TODO: Remove after testing
    float4 Channel_Debug_Position  : SV_Target3;
};

[RootSignature(GBufferPassRS)]
PSOutput main(PSInput input)
{
    MaterialData material = LoadMaterial(input.MaterialID);
    
    // -- Collect Material Data ---
    // TODO: Add default WHITE texture (1,1,1,1) so that we can avoid branching.
    // Example of the look up: ao = material.AO * texture[material.AOtexture];
    //  4x4 dxt1 white texture
    float3 albedo = material.AlbedoColour;
    if (material.AlbedoTexture != InvalidDescriptorIndex)
    {
        albedo = ResourceHeap_GetTexture2D(material.AlbedoTexture).Sample(SamplerDefault, input.TexCoord).xyz;
    }
    
    float metallic = material.Metalness;
    float roughness = material.Roughness;

    // -- Sample the material texture ---
    if (material.MaterialTexture != InvalidDescriptorIndex)
    {
        float4 materialSample = ResourceHeap_GetTexture2D(material.MaterialTexture).Sample(SamplerDefault, input.TexCoord);
        metallic = materialSample.b;
        roughness = materialSample.g;
    }
    /* Legacy stuff - add define to drive this 
    // -- Sample the individual metalness texture if there is one ---
    if (material.MetalnessTexture != InvalidDescriptorIndex)
    {
        metallic = ResourceHeap_GetTexture2D(material.MetalnessTexture).Sample(SamplerDefault, input.TexCoord).b;
    }

    // -- Sample the individual Roughtness texture if there is one ---
    if (material.RoughnessTexture != InvalidDescriptorIndex)
    {
        roughness = ResourceHeap_GetTexture2D(material.RoughnessTexture).Sample(SamplerDefault, input.TexCoord).g;
    }
      */  
    float ao = material.AO;
    if (material.AOTexture != InvalidDescriptorIndex)
    {
        ao = ResourceHeap_GetTexture2D(material.AOTexture).Sample(SamplerDefault, input.TexCoord).r;
    }

    float3 tangent = normalize(input.TangentWS.xyz);
    float3 normal = normalize(input.NormalWS);
    float3 biTangent = cross(normal, tangent.xyz) * input.TangentWS.w;

    // float3 N = normalize(input.NormalWS);
    // float3 T = normalize(input.TangentWS.w - dot(input.TangentWS.w, N) * N);
    // float3 B = cross(N, T);

    if (material.NormalTexture != InvalidDescriptorIndex)
    {
        float3x3 tbn = float3x3(tangent, biTangent, normal);
        // float3x3 tbn = float3x3(T, B, N);
        normal = ResourceHeap_GetTexture2D(material.NormalTexture).Sample(SamplerDefault, input.TexCoord).rgb * 2.0 - 1.0;
        normal = normalize(normal);
        normal = normalize(mul(normal, tbn));

#ifdef __HACK_FLIP_X_COORD
         normal.x = -normal.x;
#endif
#ifdef __HACK_FLIP_Y_COORD
         normal.y = -normal.y;
#endif
#ifdef __HACK_FLIP_Z_COORD
         normal.z = -normal.z;
#endif
    }

    // -- End Material Collection ---
    PSOutput output;
    output.Channel_0 = float4(albedo, 1.0f);
    output.Channel_1 = float4(normal, 1.0f);
    output.Channel_2 = float4(metallic, roughness, ao, 1.0f);
    output.Channel_Debug_Position = float4(input.PositionWS, 1.0f);

    return output;
}
#endif
#endif