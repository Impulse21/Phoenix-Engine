#include "Defines.hlsli"

struct DrawPushConstant
{
    matrix ModelViewProjection;
    float3 Colour;
};

ConstantBuffer<DrawPushConstant> DrawPushConstantCB : register(b0);

struct VsOutput
{
    float4 Colour : COLOR;
    float4 Position     : SV_POSITION;
};

VsOutput main(
    in float3 position: POSITION)
{
    VsOutput output;

    output.Position = mul(float4(position, 1.0f), DrawPushConstantCB.ModelViewProjection);
    output.Colour = float4(DrawPushConstantCB.Colour, 1.0f);
    
    return output;
}