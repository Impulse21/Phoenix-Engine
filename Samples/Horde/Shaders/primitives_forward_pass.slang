
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

[shader("fragment")]
float4 FS_Main<M : IMaterial>(MeshVertexOutput input): SV_Target
{
    float3 light_dir    = normalize(float3(0.5, 1.0, -0.5));
    float3 light_colour = float3(1.0, 1.0, 1.0);
    float3 view_dir     = normalize(float3(0.0, -1.0, 0.0));

    float3 sun_color     = float3(1.0f, 0.95f, 0.85f);
    float  sun_intensity = 0.8f;


    Surface      surface = DefaultSurface();
    surface.normal       = input.normal;
    surface.view_dir     = view_dir;
    surface.albedo       = input.colour;
    surface.roughness    = 0.5f;
    surface.metalness    = 0.0f;

    LightingData light_data;
    light_data.light.direction = light_dir;
    light_data.light.radiance  = sun_color * sun_intensity;

    float3 direct_lighting = shadeSurface<M>(surface, light_data);

    // Ambient
    float3 ambient = 0.1 * input.colour;

    float3 final_colour = ambient + direct_lighting;
    return float4(final_colour, 1.0);
}
