module PrimitivesCommon;

#include "primitives_interop.h"

public struct MeshVertexOutput
{
    public float4 position : SV_Position;
    public float4 world_pos : TEXCOORD1; 
    public float3 normal   : NORMAL0;
    public float3 colour   : COLOR0;
};


