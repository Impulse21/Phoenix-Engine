#ifndef __PHX_VERTEX_BUFFER_HLSLI__
#define __PHX_VERTEX_BUFFER_HLSLI__

#include "../Include/PhxEngine/Shaders/ShaderInteropStructures.h"
#include "ResourceHeapTables.hlsli"
struct VertexData
{
    float4 Position;
    float3 Normal;
    float2 TexCoord;
    float4 Tangent;
};

inline float4 RetrieveVertexPosition(in uint vertexId, in Geometry geometry)
{
    ByteAddressBuffer vertexBuffer = ResourceHeap_GetBuffer(geometry.VertexBufferIndex);
    return float4(asfloat(vertexBuffer.Load3(geometry.PositionOffset + vertexId * 12)), 1.0f);
}

inline VertexData RetrieveVertexData(in uint vertexId, in Geometry geometry)
{
    ByteAddressBuffer vertexBuffer = ResourceHeap_GetBuffer(geometry.VertexBufferIndex);
    
    VertexData output;
    output.Position = RetrieveVertexPosition(vertexId, geometry);
    output.Normal = geometry.NormalOffset == ~0u ? 0 : asfloat(vertexBuffer.Load3(geometry.NormalOffset + vertexId * 12));
    output.TexCoord = geometry.TexCoordOffset == ~0u ? 0 : asfloat(vertexBuffer.Load2(geometry.TexCoordOffset + vertexId * 8));
    output.Tangent = geometry.TangentOffset == ~0u ? 0 : asfloat(vertexBuffer.Load4(geometry.TangentOffset + vertexId * 16));
    
    return output;
}

#endif