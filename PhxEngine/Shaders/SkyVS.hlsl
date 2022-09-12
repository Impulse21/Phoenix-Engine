
#include "Sky.hlsli"
#include "FullScreenHelpers.hlsli"


struct PSInput
{
	float2 ClipSpace : TEXCOORD;
	float4 Position : SV_POSITION;
};

[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
PSInput main(uint id : SV_VertexID)
{
    PSInput output;

    float2 temp;
    CreateFullScreenTriangle(id, output.Position, temp);
    output.Position.z = 1.0f;

    output.ClipSpace = output.Position.xy;

    return output;
}
