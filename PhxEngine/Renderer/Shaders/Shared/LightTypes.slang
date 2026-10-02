// LightTypes.slang
module LightTypes;
import Surface;          // for Light

public static const uint LIGHT_DIRECTIONAL = 0;
public static const uint LIGHT_POINT       = 1;
public static const uint LIGHT_SPOT        = 2;

// GPU buffer layout. The C++ struct must match this exactly.
public struct LightData
{
    public float3 position;
    public float range;
    public float3 direction;
    public uint  type;       // direction = way the light travels
    public float3 color;
    public float intensity;
    public float  cos_inner;
    public float cos_outer;   // spot only
};

float RangeWindow(float dist, float range)             // internal helper
{
    float t = saturate(1.0 - pow(dist / range, 4.0));
    return t * t;
}

public Light EvaluateLight(LightData ld, float3 worldPos)
{
    // directional / point / spot branches, as before
}