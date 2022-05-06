#include "ResourceHeapTables.hlsli"

#include "../Include/Shaders/ShaderInterop.h"
#include "../Include/Shaders/ShaderInteropStructures.h"


// Linear clamp sampler.
SamplerState LinearClampSampler : register(s0);

// -- Const buffers ---
PUSH_CONSTANT(push, FontDrawInfo);

struct PSInput
{
    float2 TexCoord : TEXCOORD;
};

float4 main(PSInput input) : SV_Target
{
    float4 outColour = push.GetColour() * ResourceHeap_Texture2D[push.TextureIndex].SampleLevel(LinearClampSampler, input.TexCoord, 0);
    return outColour;
}
