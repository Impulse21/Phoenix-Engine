module Surface;

public struct Surface
{
    public float3 normal;     // World space, normalized,
    public float3 view_dir;   // World space, normalized,
    public float3 tangent;    // For anisoptropy. Not defined yet
    public float3 bitangent;  // For anisoptropy. Not defined yet
    public float3 albedo;
    public float  roughness;  // [0, 1] range, 0 = smooth, 1 = rough per. Note 0 will break the GGX distribution, so we clamp to 0.05f in the shader.
    public float metalness;
    public float ao;
    public float emissive;
}

public Surface DefaultSurface()
{
    Surface result;

    result.normal    = 0;
    result.view_dir  = 1;
    result.tangent   = 0;
    result.bitangent = 0;
    result.albedo    = 0;
    result.roughness = 0;
    result.metalness = 0;
    result.ao        = 1;
    result.emissive  = 0;

    return result;
}

public struct Light
{
    public float3 direction;  // direction to light, normalized,;
    public float3 radiance;   // Li, alread attenuated
}
