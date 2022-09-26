
#include "Sky.hlsli"

struct PSInput
{
	float2 TexCoords    : TEXCOORD;
	uint RTIndex        : SV_RenderTargetArrayIndex;
};

inline float3 UvToDirection(float2 uv, uint face)
{
    float3 direction = float3(uv.x * 2 - 1, 1 - uv.y * 2, 1);

    // Inverse cube face view matrices
    switch (face)
    {
        /*
    case 0: direction.xyz = float3(direction.z, direction.y, -direction.x); break;
    case 1: direction.xyz = float3(-direction.z, direction.y, direction.x); break;
    case 2: direction.xyz = float3(direction.x, direction.z, -direction.y); break;
    case 3: direction.xyz = float3(direction.x, -direction.z, direction.y); break;
    case 4: direction.xyz = float3(direction.x, direction.y, direction.z);  break;
    case 5: direction.xyz = float3(-direction.x, direction.y, -direction.z); break;
    */
    case 0: direction.xyz = float3(1, 0, 0); break;
    case 1: direction.xyz = float3(-1, 0, 0); break;
    case 2: direction.xyz = float3(0, 1, 0); break;
    case 3: direction.xyz = float3(0, -1, 0); break;
    case 4: direction.xyz = float3(0, 0, 1);  break;
    case 5: direction.xyz = float3(0, 0, -1); break;
    }

    return normalize(direction);
}

[RootSignature(PHX_ENGINE_SKY_CAPTURE_ROOTSIGNATURE)]
float4 main(PSInput input) : SV_TARGET
{

    float3 normal = UvToDirection(input.TexCoords, input.RTIndex);
	float4 colour = float4(GetProceduralSkyColour(normal), 1);
	return colour;
    // return float4((normal.x * 2.0f -1.0f), (normal.y * 2.0f - 1.0f), (normal.z * 2.0f - 1.0f), 1.0f);
}
