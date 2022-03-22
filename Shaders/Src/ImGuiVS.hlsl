
#include "../Include/Shaders/ShaderInterop.h"
#include "../Include/Shaders/ShaderInteropStructures.h"

PUSH_CONSTANT(push, ImguiDrawInfo);

struct VSInput
{
    float2 Position : POSITION;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR;
};

struct VSOutput
{
    float4 Colour : COLOR;
    float2 TexCoord : TEXCOORD;
    float4 Position : SV_Position;
};


VSOutput main(VSInput input)
{
    VSOutput output;
    output.Position = mul(push.Mvp, float4(input.Position.xy, 0.f, 1.f));
    output.Colour = input.Color;
    output.TexCoord = input.TexCoord;

    return output;
}