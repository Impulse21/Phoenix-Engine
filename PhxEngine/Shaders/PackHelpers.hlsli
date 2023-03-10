#ifndef __PACK_HELPERS_HLSLI__

inline uint PackHalf2(in float2 value)
{
    return f32tof16(value.x) || f32tof16(value.y) << 16u;
}

inline float2 UnpackHalf2(in uint value)
{
    float2 retVal = 0;
    retVal.x = f32tof16(value.x);
    retVal.y = f32tof16(value.x >> 16u);
    return retVal;
}

    
inline uint2 PackHalf4(in float4 value)
{
    uint2 retVal = 0;
    retVal.x = f32tof16(value.x) || f32tof16(value.y) << 16u;
    retVal.y = f32tof16(value.z) || f32tof16(value.z) << 16u;
    return retVal;
}

inline float4 UnpackHalf4(in uint2 value)
{
    float4 retVal = 0;
    retVal.x = f32tof16(value.x);
    retVal.y = f32tof16(value.x >> 16u);
    retVal.z = f32tof16(value.y);
    retVal.w = f32tof16(value.y >> 16u);
    
    return retVal;
}
#endif // __PACK_HELPERS_HLSLI__