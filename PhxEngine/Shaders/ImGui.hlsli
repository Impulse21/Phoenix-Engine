#ifndef __IMGUI__HLSLI__
#define __IMGUI__HLSLI__

#include "ShaderInterop.h"
#include "ShaderInteropStructures.h"
#include "ResourceHeapTables.hlsli"

#define ImGuiRS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED )," \
                "RootConstants(num32BitConstants=17, b999), " \
                RS_BINDLESS_DESCRIPTOR_TABLE \
                "StaticSampler(s0, " \
                                "addressU = TEXTURE_ADDRESS_WRAP," \
                                "addressV = TEXTURE_ADDRESS_WRAP," \
                                "addressW = TEXTURE_ADDRESS_WRAP," \
                                "filter = FILTER_MIN_MAG_MIP_LINEAR," \
                                "comparisonFunc = COMPARISON_ALWAYS, " \
                                "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK)"


PUSH_CONSTANT(push, ImguiDrawInfo);

struct VSInput
{
    float2 Position : POSITION;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR;
};

struct PSInput
{
    float4 Colour : COLOR;
    float2 TexCoord : TEXCOORD;
    float4 Position : SV_Position;
};

#ifdef IMGUI_COMPILE_VS

[RootSignature(ImGuiRS)]
PSInput main(VSInput input)
{
    PSInput output;
    output.Position = mul(push.Mvp, float4(input.Position.xy, 0.f, 1.f));
    output.Colour = input.Color;
    output.TexCoord = input.TexCoord;

    return output;
}
#endif

#ifdef IMGUI_COMPILE_PS

SamplerState LinearClampSampler : register(s0);

[RootSignature(ImGuiRS)]
float4 main(PSInput input) : SV_Target
{
    float4 outColour = input.Colour * ResourceHeap_GetTexture(push.TextureIndex).SampleLevel(LinearClampSampler, input.TexCoord, 0);
    return outColour;
}
#endif

#endif 