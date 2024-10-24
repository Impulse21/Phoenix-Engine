#pragma pack_matrix(row_major)

#define CONSTANT_BUFFER(name, type) ConstantBuffer<type> name : register(b999)
#define PUSH_CONSTANT(name, type) ConstantBuffer<type> name : register(b999)
#define InvalidDescriptorIndex ~0U
#define RS_PUSH_CONSTANT "CBV(b999, space = 1, flags = DATA_STATIC)"

#define ImGuiRS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED )," \
                "RootConstants(num32BitConstants=17, b999), " \
                "StaticSampler(s0, " \
                                "addressU = TEXTURE_ADDRESS_WRAP," \
                                "addressV = TEXTURE_ADDRESS_WRAP," \
                                "addressW = TEXTURE_ADDRESS_WRAP," \
                                "filter = FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR," \
                                "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK)"


struct ImguiDrawInfo
{
    float4x4 Mvp;
    uint TextureIndex;
};

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

[RootSignature(ImGuiRS)]
PSInput MainVS(VSInput input)
{
    PSInput output;
    output.Position = mul(float4(input.Position.xy, 0.f, 1.f), push.Mvp);
    output.Colour = input.Color;
    output.TexCoord = input.TexCoord;

    return output;
}

inline Texture2D ResourceHeap_GetTexture(uint index)
{
	return ResourceDescriptorHeap[index];
}

SamplerState LinearClampSampler : register(s0);

float4 MainPS(PSInput input) : SV_Target
{
    float4 textureColour = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (push.TextureIndex != InvalidDescriptorIndex)
    {
        textureColour =  ResourceHeap_GetTexture(push.TextureIndex).Sample(LinearClampSampler, input.TexCoord);
    }

    return input.Colour * textureColour;
}