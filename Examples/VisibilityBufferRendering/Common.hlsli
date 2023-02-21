#ifndef __COMMON_HLSL__
#define __COMMON_HLSL__


#define ROOT_SIG \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=68, b999), " \
	"CBV(b0), "


struct Camera
{
    float4x4 ViewProjection;

		// -- 16 byte boundary ----
    float4x4 ViewProjectionInv;

		// -- 16 byte boundary ----
    float4x4 ProjInv;

		// -- 16 byte boundary ----
    float4x4 ViewInv;

		// -- 16 byte boundary ----

    float4x4 ShadowViewProjection;

		// -- 16 byte boundary ----

#ifndef __cplusplus
    inline float3 GetPosition()
    {
        return ViewInv[3].xyz;
    }
#endif 
};

	
struct MeshletPushConstants
{
    float4x4 WorldMatrix;
    uint VerticesBufferIdx;
    uint MeshletsBufferIdx;
    uint UniqueVertexIBIdx;
    uint PrimitiveIndicesIdx;
};

ConstantBuffer<Camera> CameraCB : register(b0);
ConstantBuffer<MeshletPushConstants> push : register(b999);

struct VertexOut
{
    uint MeshletIndex : COLOR0;
    float3 Normal : NORMAL0;
    float3 PositionWS : POSITION0;
    float4 Position : SV_Position;
};

struct Meshlet
{
    uint VertCount;
    uint VertOffset;
    uint PrimCount;
    uint PrimOffset;
};

struct VertexData
{
    float4 Position;
    float3 Normal;
    float2 TexCoord;
    float4 Tangent;
};

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