#ifndef __BRDF_FUNCTIONS__HLSLI__
#define __BRDF_FUNCTIONS__HLSLI__

#include "Defines.hlsli"

// Constant normal incidence Fresnel factor for all dielectrics.
static const float Fdielectric = 0.04f;
static const float MaxReflectionLod = 7.0f;

struct SurfaceProperties
{
    float3  Albedo;
    float   Opactiy;
    float3  Normal;
    float   Roughness;
    float   Metalness;
    float   AO;
};

struct BrdfData
{
    // TODO: Look into correcting the L vector
    // Correction of light vector L for spherical / directional area lights.
    // Inspired by "Real Shading in Unreal Engine 4" by B. Karis, 
    // Also check out: Normalization for the widening of D, see the paper referenced above.

    float3 DiffuseReflectance;
    float Roughness;
    float3 SpecularF0; // Surface reflection and zero incidence.

    float3 N; // Shading Normal;
    float3 V; // Direction to viewer (or opposite direction of incident ray)
    float3 H; // Half vector (microfact Normal);
    float3 L; // Direction to light ( or direction of reflecting ray)

    float NdotL;
    float NdotV;
    float NdotH;
    float LdotH;
    float HdotV;


};

BrdfData PrepareBrdfData(
    float3 viewIncident,
    float3 lightIncident,
    SurfaceProperties surfaceProperties)
{
    BrdfData output;
    output.DiffuseReflectance = surfaceProperties.Albedo * (1.0f - surfaceProperties.Metalness);
    output.SpecularF0 = lerp(Fdielectric, surfaceProperties.Albedo, surfaceProperties.Metalness);
    output.Roughness = surfaceProperties.Roughness;

    output.N = normalize(surfaceProperties.Normal);
    output.V = -normalize(viewIncident);
    output.H = normalize(output.V + output.L);
    output.L = -normalize(lightIncident);

    output.NdotL = saturate(dot(output.N, output.L));
    output.NdotV = saturate(dot(output.N, output.V));
    output.NdotH = saturate(dot(output.N, output.H));
    output.LdotH = saturate(dot(output.L, output.V));
    output.HdotV = saturate(dot(output.H, output.V));

    return output;

}

// -- Normal Distribution functions ---
// approximates the amount the surface's microfacets are aligned to the halfway vector,
// influenced by the roughness of the surface; this is the primary function approximating the microfacets.

/* Summary:
 *    Trowbridge-Reitz GGX normal distribution function.
 *  N -> The surface Normal
 *  
*/
float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    
    float nominator = a2;
    float denominator = (NdotH2 * (a2 - 1.0) + 1.0);
    denominator = PI * denominator * denominator;
    
    return nominator / denominator;
}

// -- Normal Distribution functions END ---


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

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// -- Fresnel Equation ---
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
// -- END: Fresnel Equation ---


// -- Diffuse Radiance ---
float3 LambertDiffuse(BrdfData brdfData)
{
    return brdfData.DiffuseReflectance * (brdfData.NdotL / PI);
}

float3 CookTorranceSpecular(BrdfData brdfData)
{
    // Calculate surfaceProperties.Normal Distribution Term
    float NDF = DistributionGGX(brdfData.NdotH, brdfData.Roughness);

    // Calculate Geometry Term
    float G = GeometrySmith(brdfData.NdotV, brdfData.NdotL, brdfData.Roughness);

    // Calculate Fersnel Term
    float3 F = FresnelSchlick(brdfData.HdotV, brdfData.SpecularF0);

    // Now calculate Cook-Torrance BRDF
    float3 numerator = NDF * G * F;

    // NOTE: we add 0.0001 to the denomiator to prevent a divide by zero in the case any dot product ends up zero
    float denominator = 4.0 * saturate(brdfData.NdotV * brdfData.NdotL) + 0.0001;

    return  numerator / denominator;
}
#endif