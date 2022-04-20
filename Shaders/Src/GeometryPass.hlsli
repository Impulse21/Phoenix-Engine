#ifndef __GEOMETRY_PASS_HLSLI__
#define __GEOMETRY_PASS_HLSLI__

struct PSInput
{
    float3 NormalWS : NORMAL;
    float4 Colour : COLOUR;
    float2 TexCoord : TEXCOORD;
    float3 PositionWS : Position;
    float4 TangentWS : TANGENT;
    uint MaterialID : MATERIAL;
};

inline float3 ToneMapping(in float3 colour)
{
    return colour / (colour + float3(1.0, 1.0, 1.0));
}

inline float3 GammaCorrection(in float3 colour)
{
    return pow(colour, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
}
#endif // __GEOMETRY_PASS_HLSLI__