
#include "Sky.hlsli"
#include "FullScreenHelpers.hlsli"


struct PSInput
{
	float2 ClipSpace : TEXCOORD;
	float4 Position : SV_POSITION;
};


[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
PSInput main(uint vID : SV_VertexID)
{
    PSInput output;

    CreateFullscreenTriangle_POS(vID, output.Position);

    output.ClipSpace = output.Position.xy;

    return output;
}
