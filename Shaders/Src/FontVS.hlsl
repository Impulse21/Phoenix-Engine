
#include "../Include/Shaders/ShaderInterop.h"
#include "../Include/Shaders/ShaderInteropStructures.h"

PUSH_CONSTANT(push, FontDrawInfo);

struct VSInput
{
    float2 Position : POSITION;
    float2 TexCoord : TEXCOORD;
};

struct VSOutput
{
    float2 TexCoord : TEXCOORD;
    float4 Position : SV_Position;
};


VSOutput main(VSInput input)
{
    VSOutput output;
    output.Position = mul(push.Mvp, float4(input.Position.xy, 0.f, 1.f));
    output.TexCoord = input.TexCoord;

    return output;
}