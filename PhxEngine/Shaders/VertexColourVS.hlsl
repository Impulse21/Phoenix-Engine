#pragma pack_matrix(row_major)

#include "../Include/PhxEngine/Shaders/ShaderInteropStructures.h"

#define RS \
	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"RootConstants(num32BitConstants=20, b999), "

struct VertexInput
{
    float4 pos : POSITION;
};

struct PixelInput
{
    float4 Colour : COLOR;
    float4 Position : SV_POSITION;
};

PUSH_CONSTANT(push, MiscPushConstants);

[RootSignature(RS)]
PixelInput main(VertexInput input)
{
    PixelInput output;
    output.Position = mul(input.pos, push.Transform);
    output.Colour = push.Colour;
    
    return output;
}