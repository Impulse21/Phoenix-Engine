
#include "Icosphere.hlsli"
#include "Cube.hlsli"

#include "Sky.hlsli"
#include "Globals.hlsli"
#include "FullScreenHelpers.hlsli"
struct PSInput
{
    float2 TexCoords    : TEXCOORD;
    uint RTIndex        : SV_RenderTargetArrayIndex;
	float4 Position     : SV_POSITION;
};


[RootSignature(PHX_ENGINE_SKY_CAPTURE_ROOTSIGNATURE)]
PSInput main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    PSInput output;

    output.RTIndex = instanceID;

    CreateFullscreenTriangle_POS(vertexID, output.Position);
    output.TexCoords = output.Position.xy;

    // output.Position = mul(CubemapRenderCamsCB.ViewProjection[output.RTIndex], float4(sIcosphere[vertexID].xyz, 0.0f));
    // output.NormalWS = sIcosphere[vertexID].xyz;

    return output;
}
