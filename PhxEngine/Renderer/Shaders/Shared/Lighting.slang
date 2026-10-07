// forward_pass.slang

import Surface;
import MaterialInterface;

public struct LightingData
{
#if 0
    public Light* lights;
    public uint   light_count;
#else
    public Light light;
#endif
};

public float3 shadeSurface<M : IMaterial>(Surface surface, LightingData light_data)
{
    M m;
    m.prepare(surface);

#if 0
    float3 Lo = 0;
    for (uint k = 0; k < light_data.light_count; ++k)
        Lo += m.shade(surface, light_data.lights[k]);
#else
    float3 Lo = m.shade(surface, light_data.light);
#endif

    // -- Fake Ambient sky ---
    const float3 sky        = float3(0.12, 0.15, 0.20);
    const float3 ground     = float3(0.06, 0.05, 0.04);
    float3       irradiance = lerp(ground, sky, surface.normal.y * 0.5 + 0.5);

    float3       diffuse_color = surface.albedo * (1.0 - surface.metalness);
    float3       indirect      = irradiance * diffuse_color * surface.ao;
    
    return Lo + indirect + surface.emissive;
}
