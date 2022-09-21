

#include "Include/Shaders/ShaderInterop.h"
#include "Include/Shaders/ShaderInteropStructures.h"
#include "ResourceHeapTables.hlsli"
#include "CommonFunctions.hlsli"

#define GENERATE_MIP_CHAIN_ROOTSIGNATURE \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=9, b999), " \
    "StaticSampler(s0," \
           "addressU = TEXTURE_ADDRESS_CLAMP," \
           "addressV = TEXTURE_ADDRESS_CLAMP," \
           "addressW = TEXTURE_ADDRESS_CLAMP," \
           "filter = FILTER_MIN_MAG_MIP_LINEAR)"

SamplerState SamplerLinearClamp : register(s0);

PUSH_CONSTANT(push, FilterEnvMapPushConstants);

// From "Real Shading in UnrealEngine 4" by Brian Karis
// https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
float3 ImportanceSampleGGX(float2 Xi, float Roughness, float3 N)
{
    float a = Roughness * Roughness;
    float Phi = 2 * PI * Xi.x;
    float CosTheta = sqrt((1 - Xi.y) / (1 + (a * a - 1) * Xi.y));
    float SinTheta = sqrt(1 - CosTheta * CosTheta);
    float3 H;
    H.x = SinTheta * cos(Phi);
    H.y = SinTheta * sin(Phi);
    H.z = CosTheta;

    return H;
    // Compute a tangent frame and rotate the half vector to world space
    // We don't want to do this per frame, so move this out of the inner loop.
    // See where this is called in main.
    /*
    const float3 upVector = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    const float3 tangentX = normalize(cross(upVector, N));
    const float3 tangentY = corss(N, tangentX);

    // Convert Tangent to World Space
    return tangentX * H.x + tangentY * H.y + N * H.z;
    */
}


// A uniform 2D random generator for hemisphere sampling: http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
//	idx	: iteration index
//	num	: number of iterations in total
inline float2 Hammersley(uint idx, uint num) {
    uint bits = idx;
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    const float radicalInverse_VdC = float(bits) * 2.3283064365386963e-10; // / 0x100000000

    return float2(float(idx) / float(num), radicalInverse_VdC);
}


// Credit to: https://placeholderart.wordpress.com/2015/07/28/implementation-notes-runtime-environment-map-filtering-for-image-based-lighting/
[RootSignature(GENERATE_MIP_CHAIN_ROOTSIGNATURE)]
[numthreads(GENERATE_MIP_CHAIN_2D_BLOCK_SIZE, GENERATE_MIP_CHAIN_2D_BLOCK_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x < push.FilteredResolution.x && DTid.y < push.FilteredResolution.y)
    {
        TextureCubeArray input = ResourceHeap_GetTextureCubeArray(push.TextureInput);
        RWTexture2DArray<float4> output = ResourceHeap_GetRWTexture2DArray(push.TextureOutput);

        float2 uv = ((float2) DTid.xy + 0.5f) * push.FilteredResolutionRcp.xy;
        float3 N = UvToCubeMap(uv, DTid.z);

        // Calculate Tangent Space
        const float3 upVector = abs(N.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
        const float3 tangentX = normalize(cross(upVector, N));
        const float3 tangentY = cross(N, tangentX);

        float3x3 tangentToWorldMtx = float3x3(tangentX, tangentX, N);

        float4 preFilteredLightColour = 0;

        for (uint i = 0; i < push.NumSamples; ++i)
        {
            float Xi = Hammersley(i, push.NumSamples);
            float3 hemisphere = ImportanceSampleGGX(Xi, push.FilterRoughness, N);
            float3 light = mul(hemisphere, tangentToWorldMtx);

            preFilteredLightColour += input.SampleLevel(SamplerLinearClamp, float4(light, push.ArrayIndex), 0);
        }

        preFilteredLightColour /= (float)push.NumSamples;
        output[uint3(DTid.xy, DTid.z + push.ArrayIndex * 6)] = preFilteredLightColour;
    }
}