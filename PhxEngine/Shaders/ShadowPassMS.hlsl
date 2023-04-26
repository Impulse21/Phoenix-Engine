#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"

#include "Globals_New.hlsli"
#include "VertexFetch.hlsli"
#include "Defines.hlsli"
#include "GBuffer_New.hlsli"
#include "PackHelpers.hlsli"

#ifdef COMPILE_MS
#include "MeshletCommon.hlsli"
#endif

PUSH_CONSTANT(push, GeometryPushConstant);

struct PSInput
{
    float3 NormalWS : NORMAL;
    float2 TexCoord : TEXCOORD;
    float4 TangentWS : TANGENT;
    float3 Colour : COLOR0;
    float3 EmissiveColour : COLOR1;
    uint MeshletIndex : COLOR2;
    uint MaterialID : MATERIAL;
    float3 PositionWS : Position;
    float4 Position : SV_POSITION;
};
