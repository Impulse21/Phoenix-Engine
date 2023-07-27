#pragma pack_matrix(row_major)

#define RS \
	"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"RootConstants(num32BitConstants=20, b999), "

struct PixelInput
{
    float4 Colour : COLOR;
};

[RootSignature(RS)]
float4 main(PixelInput input) : SV_TARGET
{
    return input.Colour;
}