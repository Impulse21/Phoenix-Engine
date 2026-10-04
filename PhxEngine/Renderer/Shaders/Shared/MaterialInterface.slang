module MaterialInterface;

import BrdfCommon;
import Surface;

public interface IMaterial
{
    // Prepares the material for shading, given the surface properties.
    // call once per pixel.
    public [mutating] void   prepare(Surface s);

    // Shade the surface given the light properties. Call once per light.
    public float3 shade(Surface s, Light light);
}

public struct Standard : IMaterial
{
    float  roughness;
    float  alpha2;
    float  n_v;
    float  a_term;
    float3 F0;
    float3 diffuse_Color;

    // -- IMaterial interface ---
    [mutating] void prepare(Surface s)
    {
        roughness    = max(s.roughness, 0.045);

        float alpha  = roughness * roughness;
        alpha2       = alpha * alpha;

        n_v          = max(dot(s.normal, s.view_dir), 1e-4);
        a_term       = n_v + sqrt(alpha2 + n_v * (n_v - alpha2 * n_v));
        F0           = lerp(float3(0.04), s.albedo, s.metalness);
        diffuse_Color = (1 - s.metalness) * s.albedo;
    }

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
    float3 shade(Surface surface, Light light)
    {
        float3 l   = light.direction;
        float3 h   = normalize(l + surface.view_dir);
        float  n_l = saturate(dot(surface.normal, l));
        float  n_h = saturate(dot(surface.normal, h));
        float  l_h = saturate(dot(l, h));

        float3 spec = D_GGX(n_h, alpha2)
                    * V_SmithGGX_Lagarde(a_term, n_l, alpha2)
                    * F_Schlick(F0, l_h);

        float3 diff = diffuse_Color * Fd_Burley(n_l, n_v, l_h, roughness);

        return (diff + spec) * light.radiance * n_l;
    }

}