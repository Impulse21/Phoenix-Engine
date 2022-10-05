#ifndef __BRDF_FUNCTIONS__HLSLI__
#define __BRDF_FUNCTIONS__HLSLI__

#include "Defines.hlsli"

// Constant normal incidence Fresnel factor for all dielectrics.
static const float Fdielectric = 0.04f;
static const float MaxReflectionLod = 7.0f;

struct Surface
{
    float3  Albedo;
    float   Opactiy;
    float3  Normal;
    float   Roughness;
    float   Metalness;
    float   AO;
    float3  Emissive;
    float3  Specular; // Tech part of BRDF
};

Surface DefaultSurface()
{
    Surface result;

    result.Albedo = 0;
    result.Opactiy = 1;
    result.Normal = 0;
    result.Roughness = 0;
    result.Metalness = 0;
    result.AO = 1;
    result.Emissive = 0;
    result.Specular = 0;

    return result;
}

struct BRDFDataPerSurface
{
    float3 P; // World-space position
    float3 N; // Shading Normal
    float3 V; // Direction to Viewer ( or opposite direction of incident ray)
    float3 R; // Reflection Vector

    // -- Dot Products ---
    float NdotV;

    // -- Surface properties ---
    // Diffuse could probably come from the Gbuffer as well.
    float3 DiffuseReflectance; // (1 - metalness) * albedo. The albedo comes from Lambert's function Lambert = kd * (albedo/pi)
    float3 F0; // Specular at zero incident
    float F90; // Specular at 90 incident

    float Alpha;
    float Alpha2;

    float3 F; // FresnelSchlick of the surface (NdotV). Used for indirect specular
    float SmithG2Lagarde_ATerm; // Pre calculate A Term of the G function.
};

struct BRDFDataPerLight
{
    // TODO: Look into correcting the L vector
    // Correction of light vector L for spherical / directional area lights.
    // Inspired by "Real Shading in Unreal Engine 4" by B. Karis, 
    // Also check out: Normalization for the widening of D, see the paper referenced above.
    float3 L;   // Direction to light ( or direction of reflecting ray)
    float3 H;    // Half vector (microfact Normal);
    float3 F;   // FresnelSchlick of the surface (VdotH). Used for direct specular

    // -- Dot Products ---
    float NdotH;
    float NdotL;
    float LdotH;
    float HdotV;
    float VdotH;
};

// -- Normal Distribution functions ---
// approximates the amount the surface's microfacets are aligned to the halfway vector,
// influenced by the roughness of the surface; this is the primary function approximating the microfacets.

/* Summary:
 *    Trowbridge-Reitz GGX normal distribution function.
 *  N -> The surface Normal
 *  
*/
float DistributionGGX(float NdotH, float alpha2)
{
    float NdotH2 = NdotH * NdotH;

    float nominator = alpha2;
    float denominator = (NdotH2 * (alpha2 - 1.0) + 1.0);
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

float GeometrySmithG2(float NdotV, float NdotL, float roughness)
{
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Smith G2 term (masking-shadowing function) for GGX distribution
// Separable version assuming independent (uncorrelated) masking and shadowing - optimized by substituing G_Lambda for G_Lambda_GGX and 
// dividing by (4 * NdotL * NdotV) to cancel out these terms in specular BRDF denominator
// Source: "Moving Frostbite to Physically Based Rendering" by Lagarde & de Rousiers
// Note that returned value is G2 / (4 * NdotL * NdotV) and therefore includes division by specular BRDF denominator
float GeometrySmithG2_Lagarde(float smithG2Lagarde_ATerm, float NdotL, float alpha2)
{
    // Pre-calculated
    // float a = NdotV + sqrt(alpha2 + NdotV * (NdotV - alpha2 * NdotV));
    float b = NdotL + sqrt(alpha2 + NdotL * (NdotL - alpha2 * NdotL));
    return 1.0f / (smithG2Lagarde_ATerm * b);
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

float3 FresnelSchlick(const float3 F0, float F90, float VdotH)
{
    // Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
    return F0 + (F90 - F0) * pow(1.0 - VdotH, 5);
}
// -- END: Fresnel Equation ---



BRDFDataPerSurface CreatePerSurfaceBRDFData(Surface surface, float3 surfacePos, float3 viewIncidentVector)
{
    BRDFDataPerSurface output;
    output.P = surfacePos;
    output.N = normalize(surface.Normal);
    output.V = -normalize(viewIncidentVector);
    output.R = reflect(viewIncidentVector, output.N);

    output.NdotV = saturate(dot(output.N, output.V));

    output.DiffuseReflectance = (1 - max(Fdielectric, surface.Metalness)) * surface.Albedo;
    output.F0 = surface.Specular;
    output.F90 = saturate(50.0 * dot(output.F0, 0.33));
    output.Alpha = surface.Roughness * surface.Roughness;
    output.Alpha2 = output.Alpha * output.Alpha;

    output.F = FresnelSchlick(output.F0, output.F90, output.NdotV);
    output.SmithG2Lagarde_ATerm = output.NdotV + sqrt(output.Alpha2 + output.NdotV * (output.NdotV - output.Alpha2 * output.NdotV));

    return output;
}

BRDFDataPerLight CreatePerLightBRDFData(
    float3 lightIncident,
    BRDFDataPerSurface brdfSurface)
{
    BRDFDataPerLight output;

    output.L = -normalize(lightIncident);
    output.H = normalize(brdfSurface.V + output.L);

    output.NdotL = saturate(dot(brdfSurface.N, output.L));
    output.NdotH = saturate(dot(brdfSurface.N, output.H));
    output.LdotH = saturate(dot(output.L, brdfSurface.V));
    output.VdotH = saturate(dot(brdfSurface.V, output.H));
    output.HdotV = saturate(dot(output.H, brdfSurface.V));

    output.F = FresnelSchlick(brdfSurface.F0, brdfSurface.F90, output.VdotH);

    return output;
}

// -- Diffuse Radiance ---
float3 BRDF_DirectDiffuse(BRDFDataPerSurface brdfSurfaceData, BRDFDataPerLight brdfLightData)
{
    // kS = F;
    // Kd = 1 - kS = 1 - F;
    float3 kD = 1 - brdfLightData.F;

    // Pre-calculated in BRDFDataPerSurface::DiffuseReflectance to avoid doing this per light source.
    // kD = KD * (1 - metalness)

    // Lambert has been modified for optmization. Rather then calculating this per light
    // I encoded the Cdiff(albedo) into BRDFDataPerSurface::DiffuseReflectance and will divide total
    // per light diffuse by pi at the end. See "ApplyLighting"

    return kD * brdfLightData.NdotL;
}

float3 BRDF_DirectSpecular(BRDFDataPerSurface brdfSurfaceData, BRDFDataPerLight brdfLightData)
{
    // Calculate Normal Distribution Term
    float NDF = DistributionGGX(brdfLightData.NdotH, brdfSurfaceData.Alpha2);

    // Calculate Geometry Term
    float G = GeometrySmithG2_Lagarde(brdfSurfaceData.SmithG2Lagarde_ATerm, brdfLightData.NdotL, brdfSurfaceData.Alpha2);

    // Calculate Fersnel Term (Pre-calculated in BRDFDataPerLight::F
    // float3 F = FresnelSchlick(brdfSurfaceData.F0, brdfSurfaceData.F90, brdfLightData.NdotV);
    
    // Now calculate Cook-Torrance BRDF
    float3 specular = NDF * G * brdfLightData.F;

    return specular * brdfLightData.NdotL;

    // We do not need to do this as we are using GeometrySmithG2_Lagarde.
    // The returned value of G is  G2 / (4 * NdotL * NdotV) and therefore includes division by specular BRDF denominator
#if 0
    float3 numerator = NDF * G * brdfLightData.F;
    // NOTE: we add 0.0001 to the denomiator to prevent a divide by zero in the case any dot product ends up zero
    float denominator = 4.0 * saturate(brdfData.NdotV * brdfLightData.NdotL) + 0.0001;

    return  numerator / denominator;
#endif

}
#endif