
#include "Defines.hlsli"

#ifndef __BRDF_FUNCTIONS__HLSLI__
#define __BRDF_FUNCTIONS__HLSLI__

// -- Normal Distribution functions ---
// approximates the amount the surface's microfacets are aligned to the halfway vector,
// influenced by the roughness of the surface; this is the primary function approximating the microfacets.

/* Summary:
 *    Trowbridge-Reitz GGX normal distribution function.
 *  N -> The surface Normal
 *  
*/
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;
    
    float nominator = a2;
    float denominator = (NdotH2 * (a2 - 1.0) + 1.0);
    denominator = PI * denominator * denominator;
    
    return nominator / denominator;
}

// -- Geometry functions ---
// describes the self-shadowing property of the microfacets. When a surface is relatively rough, 
// the surface's microfacets can overshadow other microfacets reducing the light the surface reflects.

float GeometrySchlickGGX(float NdotV, float roughness)
{
    // Direct Lighting's K value is ( r + 1 )^2 / 8;
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nominator = NdotV;
    float denominator = NdotV * (1.0f - k) + k;
    
    return nominator / denominator;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// -- Fresnel Equation---
// The Fresnel equation describes the ratio of surface reflection at different surface angles.

// F0 is precomputed base reflectivity of the surface.
// CosTheta is the dot product of N(surface Normal) and the h(Halfway Vector)
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0);
}

float3 FresnelSchlick(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max((1.0f - roughness).xxx, F0) - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

#endif