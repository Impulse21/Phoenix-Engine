// forward_pass.slang

import Surface;
import MaterialInterface;

public struct LightingData
{
    // Only support one light for now
    public Light light;
};

public float3 shadeSurface<M : IMaterial>(Surface surface, LightingData light_data)
{
    M m;
    m.prepare(surface);

    // TODO: loop torugh lights.

    float3 Lo = m.shade(surface, light_data.light);
    return Lo;
}
