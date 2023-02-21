#include "Globals.hlsli"
#include "Defines.hlsli"

#define ROOT_SIG \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=22, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \

#define MAX_PRIMS_PER_MESHLET 256
#define MAX_VERTICES_PER_MESHLET 128
#define MAX_THREAD_COUNT 256

PUSH_CONSTANT(push, MeshletPushConstants);


struct VertexOut
{
    uint MeshletIndex : COLOR0;
    float3 Normal : NORMAL0;
    float3 PositionWS : POSITION0;
    float4 Position : SV_Position;
};

[RootSignature(ROOT_SIG)]
float4 main(VertexOut input) : SV_TARGET
{
    float ambientIntensity = 0.1;
    float3 lightColor = float3(1, 1, 1);
    float3 lightDir = -normalize(float3(1, -1, 1));

    float3 diffuseColor;
    float shininess;
    uint meshletIndex = input.MeshletIndex;
    if (meshletIndex == 0)
    {
        meshletIndex += 1;
    }
    
    diffuseColor = float3(
            float(meshletIndex & 1),
            float(meshletIndex & 3) / 4,
            float(meshletIndex & 7) / 8);
    shininess = 16.0;
    float3 normal = normalize(input.Normal);

    // Do some fancy Blinn-Phong shading!
    float cosAngle = saturate(dot(normal, lightDir));
    float3 viewDir = -normalize(input.PositionWS);
    float3 halfAngle = normalize(lightDir + viewDir);

    float blinnTerm = saturate(dot(normal, halfAngle));
    blinnTerm = cosAngle != 0.0 ? blinnTerm : 0.0;
    blinnTerm = pow(blinnTerm, shininess);

    float3 finalColor = (cosAngle + blinnTerm + ambientIntensity) * diffuseColor;

    return float4(finalColor, 1);
}
