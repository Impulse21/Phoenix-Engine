#pragma pack_matrix(row_major)

#include "Icosphere.hlsli"
#include "Sky.hlsli"
#include "Globals.hlsli"

#include "Cube.hlsli"
struct PSInput
{
    float3 NormalWS     : NORMAL;
    float4 Position     : SV_POSITION;
    uint RTIndex        : SV_RenderTargetArrayIndex;
};


[RootSignature(PHX_ENGINE_SKY_CAPTURE_ROOTSIGNATURE)]
PSInput main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    PSInput output;

    output.RTIndex = instanceID;

    output.Position = mul(float4(sIcosphere[vertexID].xyz, 0.0f), CubemapRenderCamsCB.ViewProjection[output.RTIndex]);
    output.NormalWS = sIcosphere[vertexID].xyz;

    return output;
}
