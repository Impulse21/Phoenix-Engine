
#include "Globals.hlsli"
#include "BRDFFunctions.hlsli"
#include "GeometryPass.hlsli"

float4 main(PSInput input) : SV_Target
{
    MaterialData material = LoadMaterial(input.MaterialID);
    
    // -- Collect Material Data ---
    float3 colour = material.GetEmissiveColour().rgb;

    colour = ToneMapping(colour);
    colour = GammaCorrection(colour);
    
    return float4(colour, 1.0f);
}