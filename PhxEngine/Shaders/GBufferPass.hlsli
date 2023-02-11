#ifndef __PHX_GBUFFER_PASS_HLSLI__
#define __PHX_GBUFFER_PASS_HLSLI__

#include "Globals.hlsli"
#include "Defines.hlsli"
#include "GBuffer.hlsli"
#include "VertexBuffer.hlsli"

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

inline Geometry GetGeometry()
{
    return LoadGeometry(push.GeometryIndex);
}

inline MaterialData GetMaterial()
{
    return LoadMaterial(push.MaterialIndex);
}

inline ShaderMeshInstancePointer GetMeshInstancePtr(uint index)
{
    return ResourceHeap_GetBuffer(push.InstancePtrBufferDescriptorIndex).Load<ShaderMeshInstancePointer>(push.InstancePtrDataOffset + index * sizeof(ShaderMeshInstancePointer));
}

struct VertexInput
{
    uint VertexID : SV_VertexID;
    uint InstanceID : SV_InstanceID;
};

    
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

    // Get Instance Data

    Geometry geometry = GetGeometry();

    VertexData vertexData = RetrieveVertexData(input.VertexID, GetGeometry());

    ShaderMeshInstancePointer instancePtr = GetMeshInstancePtr(input.InstanceID);
    MeshInstance meshInstance = LoadMeshInstance(instancePtr.GetInstanceIndex());
    meshInstance = ResourceHeap_GetBuffer(GetScene().MeshInstanceBufferIndex).Load<MeshInstance>(instancePtr.GetInstanceIndex() * sizeof(MeshInstance));

    matrix worldMatrix = meshInstance.WorldMatrix;

    output.PositionWS = mul(vertexData.Position, worldMatrix).xyz;
    output.Position = mul(float4(output.PositionWS, 1.0f), GetCamera().ViewProjection);

    output.NormalWS = mul(vertexData.Normal, worldMatrix).xyz;
    output.TexCoord = vertexData.TexCoord;
    output.Colour = float4(1.0f, 1.0f, 1.0f, 1.0f);

    output.TangentWS = float4(mul(vertexData.Tangent.xyz, (float3x3)worldMatrix), vertexData.Tangent.w);

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
    float4 Channel_3    : SV_Target3;
};

[RootSignature(GBufferPassRS)]
PSOutput main(PSInput input)
{
    MaterialData material = LoadMaterial(input.MaterialID);
    
    Surface surface = DefaultSurface();

    // -- Collect Material Data ---
    // TODO: Add default WHITE texture (1,1,1,1) so that we can avoid branching.
    // Example of the look up: ao = material.AO * texture[material.AOtexture];
    //  4x4 dxt1 white texture

    // Got this from Wicked engine.
    // Better apply the vertex base colour with the albedo info.
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
    /* Legacy stuff - add define to drive this 
    // -- Sample the individual metalness texture if there is one ---
    if (material.MetalnessTexture != InvalidDescriptorIndex)
    {
        surface.Metalness = ResourceHeap_GetTexture2D(material.MetalnessTexture).Sample(SamplerDefault, input.TexCoord).b;
    }

    // -- Sample the individual Roughtness texture if there is one ---
    if (material.RoughnessTexture != InvalidDescriptorIndex)
    {
        surface.Roughness = ResourceHeap_GetTexture2D(material.RoughnessTexture).Sample(SamplerDefault, input.TexCoord).g;
    }
      */  
    surface.AO = material.AO;
    if (material.AOTexture != InvalidDescriptorIndex)
    {
        surface.AO = ResourceHeap_GetTexture2D(material.AOTexture).Sample(SamplerDefault, input.TexCoord).r;
    }

    float3 tangent = normalize(input.TangentWS.xyz);
    surface.Normal = normalize(input.NormalWS);
    float3 biTangent = cross(surface.Normal, tangent.xyz) * input.TangentWS.w;

    // float3 N = normalize(input.NormalWS);
    // float3 T = normalize(input.TangentWS.w - dot(input.TangentWS.w, N) * N);
    // float3 B = cross(N, T);

    if (material.NormalTexture != InvalidDescriptorIndex)
    {
        float3x3 tbn = float3x3(tangent, biTangent, surface.Normal);
        // float3x3 tbn = float3x3(T, B, N);
        surface.Normal = ResourceHeap_GetTexture2D(material.NormalTexture).Sample(SamplerDefault, input.TexCoord).rgb * 2.0 - 1.0;
        surface.Normal = normalize(surface.Normal);
        surface.Normal = normalize(mul(surface.Normal, tbn));

#ifdef __HACK_FLIP_X_COORD
        surface.Normal.x = -surface.Normal.x;
#endif
#ifdef __HACK_FLIP_Y_COORD
        surface.Normal.y = -surface.Normal.y;
#endif
#ifdef __HACK_FLIP_Z_COORD
        surface.Normal.z = -surface.Normal.z;
#endif
    }

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
#endif
#endif