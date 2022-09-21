
#include "Icosphere.hlsli"
#include "Sky.hlsli"
#include "Globals.hlsli"

struct PSInput
{
    float3 NormalWS     : NORMAL;
    uint RTIndex        : SV_RenderTargetArrayIndex;
	float4 Position     : SV_POSITION;
};

[RootSignature(PHX_ENGINE_SKY_CAPTURE_ROOTSIGNATURE)]
PSInput main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    PSInput output;

    output.RTIndex = instanceID;
    output.Position = mul(CubemapRenderCamsCB.ViewProjection[output.RTIndex], float4(sIcosphere[vertexID].xyz, 0));
    output.NormalWS = sIcosphere[vertexID].xyz;

    return output;
}
