#ifndef _DEFERRED_LIGHTING_HLSL__
#define _DEFERRED_LIGHTING_HLSL__

#include "Globals.hlsli"
#include "GBuffer.hlsli"
#include "FullScreenHelpers.hlsli"
#include "Lighting.hlsli"

#ifdef ENABLE_THREAD_GROUP_SWIZZLING
#include "ThreadGroupTilingX.hlsli"
#endif

// TODO: ROOT SIGNATURE

#if USE_RESOURCE_HEAP

#define DeferredLightingRS \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "SRV(t0),"  \
    "DescriptorTable(SRV(t1, numDescriptors = 4)), " \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR),"

#else

#define DeferredLightingRS \
	"RootFlags(0), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "SRV(t0),"  \
    "DescriptorTable(SRV(t1, numDescriptors = 4)), " \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR),"

#endif

#ifdef DEFERRED_LIGHTING_COMPILE_CS

#define RS_Deffered \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=3, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "SRV(t0),"  \
    "DescriptorTable(SRV(t1, numDescriptors = 4)), " \
    "DescriptorTable( UAV(u0, numDescriptors = 1) )," \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR),"


RWTexture2D<float4> OutputBuffer : register(u0);

// Root Constant

PUSH_CONSTANT(push, DefferedLightingCSConstants);
#endif

Texture2D GBuffer_Depth : register(t1);
Texture2D GBuffer_0     : register(t2);
Texture2D GBuffer_1     : register(t3);
Texture2D GBuffer_2     : register(t4);

SamplerState DefaultSampler: register(s50);

struct PSInput
{
	float2 UV : TEXCOORD;
	float4 Position : SV_POSITION;
};

#ifdef DEFERRED_LIGHTING_COMPILE_VS
[RootSignature(DeferredLightingRS)]
PSInput main(uint id : SV_VertexID)
{
    PSInput output;

    CreateFullscreenTriangle_POS_UV(id, output.Position, output.UV);

    return output;
}

#endif

#if defined(DEFERRED_LIGHTING_COMPILE_PS) || defined(DEFERRED_LIGHTING_COMPILE_CS)

#ifdef DEFERRED_LIGHTING_COMPILE_CS

[RootSignature(RS_Deffered)]
[numthreads(DEFERRED_BLOCK_SIZE_X, DEFERRED_BLOCK_SIZE_Y, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint2 GTid : SV_GroupThreadID, uint2 GroupId : SV_GroupID)
{
#ifdef ENABLE_THREAD_GROUP_SWIZZLING
    float2 pixelPosition = ThreadGroupTilingX(
        push.DipatchGridDim,
        uint2(DEFERRED_BLOCK_SIZE_X, DEFERRED_BLOCK_SIZE_Y),
        push.MaxTileWidth,		// User parameter (N). Recommended values: 
        GTid,		// SV_GroupThreadID
        GroupId);			// SV_GroupID

#else
    float2 pixelPosition = DTid.xy;
#endif 

#else
[RootSignature(DeferredLightingRS)]
float4 main(PSInput input) : SV_TARGET
{
    float2 pixelPosition = input.UV;

#endif
    Camera camera = GetCamera();
    Scene scene = GetScene();

    float4 gbufferChannels[NUM_GBUFFER_CHANNELS];

#ifdef DEFERRED_LIGHTING_COMPILE_CS
    gbufferChannels[0] = GBuffer_0[pixelPosition];
    gbufferChannels[1] = GBuffer_1[pixelPosition];
    gbufferChannels[2] = GBuffer_2[pixelPosition];

#else
    gbufferChannels[0] = GBuffer_0.Sample(SamplerDefault, pixelPosition);
    gbufferChannels[1] = GBuffer_1.Sample(SamplerDefault, pixelPosition);
    gbufferChannels[2] = GBuffer_2.Sample(SamplerDefault, pixelPosition);
#endif
    SurfaceProperties surfaceProperties = DecodeGBuffer(gbufferChannels);
    float depth = GBuffer_Depth.Sample(SamplerDefault, pixelPosition).x;
    float3 surfacePosition = ReconstructWorldPosition(camera, pixelPosition, depth);
    float3 viewIncident = surfacePosition - (float3)GetCamera().CameraPosition;
    const float shadow = 1.0f;
	Lighting lightingTerms;

    // -- Collect Direct Light contribution ---
	[loop]
	for (int nLights = 0; nLights < scene.NumLights; nLights++)
	{
		ShaderLight light = LoadLight(nLights);
		LightingPart directRadiance;
		directRadiance.Diffuse = 0;
		directRadiance.Specular = 0;

		ShadeSurface_Direct(light, surfaceProperties, surfacePosition, viewIncident, directRadiance);


        lightingTerms.Direct.Diffuse += (shadow * directRadiance.Diffuse) * light.GetColour().rgb;
        lightingTerms.Direct.Specular += (shadow * directRadiance.Specular) * light.GetColour().rgb;
	}

    // -- Collect Indirect Light Contribution ---
    // 
    // Improvised abmient lighting by using the Env Irradance map.

    float3 N = normalize(surfaceProperties.Normal);
    float3 V = normalize((float3)GetCamera().CameraPosition - surfacePosition);
    float3 F0 = lerp(Fdielectric, surfaceProperties.Albedo, surfaceProperties.Metalness);

    float3 ambient = float(0.01).xxx;

    if (GetScene().EnvMapArray != InvalidDescriptorIndex)
    {
        float lodLevel = GetScene().EnvMap_NumMips;
        ambient += ResourceHeap_GetTextureCubeArray(GetScene().EnvMapArray).SampleLevel(SamplerLinearClamped, float4(N, 0), lodLevel).rgb;
    }

    float3 F = FresnelSchlick(saturate(dot(N, V)), F0, surfaceProperties.Roughness);
    // Improvised abmient lighting by using the Env Irradance map.
    float3 irradiance = float3(0, 0, 0);
    if (GetScene().IrradianceMapTexIndex != InvalidDescriptorIndex)
    {
        irradiance = ResourceHeap_GetTextureCube(GetScene().IrradianceMapTexIndex).Sample(SamplerDefault, N).rgb;
    }

    float3 kSpecular = F;
    float3 kDiffuse = 1.0 - kSpecular;
    float3 diffuse = irradiance * surfaceProperties.Albedo;

	// Sample both the BRDFLut and Pre-filtered map and combine them together as per the
	// split-sum approximation to get the IBL Specular part.
	float3 prefilteredColour = float3(0.0f, 0.0f, 0.0f);
	if (GetScene().EnvMapArray != InvalidDescriptorIndex)
	{
		float lodLevel = surfaceProperties.Roughness * MaxReflectionLod;
		prefilteredColour = ResourceHeap_GetTextureCubeArray(GetScene().EnvMapArray).SampleLevel(SamplerLinearClamped, float4(N, 0), lodLevel).rgb;
	}

	float2 brdfTexCoord = float2(saturate(dot(N, V)), surfaceProperties.Roughness);
	float2 brdf = float2(1.0f, 1.0f);
	if (FrameCB.BrdfLUTTexIndex != InvalidDescriptorIndex)
	{
		brdf = ResourceHeap_GetTexture2D(FrameCB.BrdfLUTTexIndex).Sample(SamplerLinearClamped, brdfTexCoord).rg;
	}

    lightingTerms.Indirect.Specular = prefilteredColour * (F * brdf.x + brdf.y);
    lightingTerms.Indirect.Diffuse = ambient;

	float3 finalColour = ApplyLighting(lightingTerms);
;

#ifdef DEFERRED_LIGHTING_COMPILE_CS
    OutputBuffer[pixelPosition] = float4(finalColour, 1.0f);
#else

    return float4(finalColour, 1.0f);
#endif
}
#endif

#endif // _DEFERRED_LIGHTING_HLSL__