
/*
TODO: Move to a generaic set of functions that can be shared between fragment Shaders.
fsMain (pass)
 ├─ BuildSurface(input, material)      → Surface         (geometry + material data only)
 └─ shadeSurface<M>(surface, lighting)
     ├─ m.prepare(surface)             once per pixel
     └─ for each light:
         ├─ EvaluateLight(lightData, surface.position)   → Light
         └─ m.shade(surface, light)
*/

import PrimitivesCommon;
import MaterialInterface;
import Surface;
import Lighting;

[[vk::push_constant]]
ConstantBuffer<FrameData*> g_root;

[shader("fragment")]
float4 FS_Main<M : IMaterial>(MeshVertexOutput input): SV_Target
{
    const float3 light_dir    = normalize(float3(0.5, 1.0, -0.5));
    const float3 light_colour = float3(1.0, 1.0, 1.0);

    const float3 camera_pos = g_root.camera_pos;
    const float3 world_pos  = input.world_pos.xyz;
    const float3 view_dir   = normalize(camera_pos - world_pos);

    const float3 sun_color     = float3(1.0f, 0.95f, 0.85f);
    const float  sun_intensity = 3.0f ;

    Surface      surface = DefaultSurface();
    surface.normal       = input.normal;
    surface.view_dir     = view_dir;
    surface.albedo       = input.colour;
    surface.roughness    = 0.5f;
    surface.metalness    = 0.0f;

    Light sun;
    sun.direction = light_dir;
    sun.radiance  = sun_color * sun_intensity;

    LightingData light_data;
    light_data.light       = sun;
    // light_data.light_count = 1;

    float3 direct_lighting = shadeSurface<M>(surface, light_data);

    // Ambient
    float3 ambient = 0.1 * input.colour;

    float3 final_colour = ambient + direct_lighting;
    return float4(final_colour, 1.0);
}
