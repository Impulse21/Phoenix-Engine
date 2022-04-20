
#include "Globals.hlsli"
#include "BRDFFunctions.hlsli"
#include "GeometryPass.hlsli"


float4 main(PSInput input) : SV_Target
{
    MaterialData material = LoadMaterial(input.MaterialID);
    
    // -- Collect Material Data ---
    float3 albedo = material.AlbedoColour;
    if (material.AlbedoTexture != InvalidDescriptorIndex)
    {
        albedo = ResourceHeap_Texture2D[material.AlbedoTexture].Sample(SamplerDefault, input.TexCoord).xyz;
    }

    albedo = ToneMapping(albedo);
    albedo = GammaCorrection(albedo);

    return float4(albedo, 1.0f);
}