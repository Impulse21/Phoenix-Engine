module BrdfCommon;

/*
    Cook-Torrance Microfacet Specular BRDF:

                       D(H) * G(V, L, H) * F(V, H)
    f_specular(L, V) = ---------------------------
                           4 * (N.V) * (N.L)

    Where:
    - D(H)       : Normal Distribution Function (NDF)
                   Measures the concentration of microfacets aligned with the Half vector (H).
                   Common implementation: GGX / Trowbridge-Reitz.

    - G(V, L, H) : Geometry Function (Shadowing / Masking)
                   Measures the probability that microfacets are either shadowed by other
                   microfacets (blocking incoming light L) or masked (blocking outgoing light V).
                   Common implementation: Smith's method paired with Schlick-GGX.

    - F(V, H)    : Fresnel Equation
                   Measures the fraction of light reflected from the surface at a given angle.
                   Defines the surface's specular reflectivity change across grazing angles.
                   Common implementation: Schlick's Approximation.
*/

// -- Normal Distribution functions ---
// approximates the amount the surface's microfacets are aligned to the halfway vector,
// influenced by the roughness of the surface; this is the primary function approximating the microfacets.

/* Summary:
 *    Trowbridge-Reitz GGX normal distribution function.
 *  N -> The surface Normal
 *
 * NOTE Can use generics if I don't want to harddcode floats
 *  SLANG: T DistributionGGX<T : __BuiltinFloatingPointType>(T NdotH, T alpha2)
 */
public float D_GGX(float NdotH, float alpha2)
{
    float NdotH2 = NdotH * NdotH;

    float nominator   = alpha2;
    float denominator = (NdotH2 * (alpha2 - 1.0) + 1.0);
    denominator       = float.getPi() * denominator * denominator;

    return nominator / denominator;
}

// -- Geometry functions ---
// describes the self-shadowing property of the microfacets. When a surface is relatively rough,
// the surface's microfacets can overshadow other microfacets reducing the light the surface reflects.

public float G_SchlickGGX(float NdotV, float roughness)
{
    // Direct Lighting's K value is ( r + 1 )^2 / 8;
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nominator   = NdotV;
    float denominator = NdotV * (1.0f - k) + k;

    return nominator / denominator;
}

public float G_SmithG2(float NdotV, float NdotL, float roughness)
{
    float ggx1 = G_SchlickGGX(NdotV, roughness);
    float ggx2 = G_SchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Smith G2 term (masking-shadowing function) for GGX distribution
// Separable version assuming independent (uncorrelated) masking and shadowing - optimized by substituing G_Lambda for G_Lambda_GGX and 
// dividing by (4 * NdotL * NdotV) to cancel out these terms in specular BRDF denominator
// Source: "Moving Frostbite to Physically Based Rendering" by Lagarde & de Rousiers
// Note that returned value is G2 / (4 * NdotL * NdotV) and therefore includes division by specular BRDF denominator
public float V_SmithGGX_Lagarde(float smithG2Lagarde_ATerm, float NdotL, float alpha2)
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
public float3 F_Schlick(float3 F0, float cosTheta)
{
    return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0);
}

public float3 F_Schlick(float3 F0, float cosTheta, float roughness)
{
    return F0 + (max((1.0f - roughness).xxx, F0) - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

// -- END: Fresnel Equation ---

// -- Disney/Burley diffuse ---

// Returns the diffuse BRDF *without* albedo (multiply by diffuseColor outside).
// nl, nv, lh are saturated dot products; roughness is perceptual (0..1), not alpha.
public float Fd_Burley(float nl, float nv, float lh, float roughness)
{
    float fd90         = 0.5 + 2.0 * roughness * lh * lh;
    float light_scatter = 1.0 + (fd90 - 1.0) * pow(1.0 - nl, 5.0);
    float view_scatter  = 1.0 + (fd90 - 1.0) * pow(1.0 - nv, 5.0);
    return light_scatter * view_scatter / float.getPi();
}