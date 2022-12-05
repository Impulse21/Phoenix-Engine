
#include "Globals.hlsli"
#include "Defines.hlsli"

#if USE_RESOURCE_HEAP
#define ShadowPassRS \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=19, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "CBV(b2),"  \
	RS_BINDLESS_DESCRIPTOR_TABLE \

#else
#define ShadowPassRS \
	"RootConstants(num32BitConstants=19, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "CBV(b2),"  \
	RS_BINDLESS_DESCRIPTOR_TABLE \

#endif 

PUSH_CONSTANT(push, GeometryPassPushConstants);


ConstantBuffer<RenderCams> RenderCams : register(b2);

[RootSignature(ShadowPassRS)]
void main(
	in uint inVertexID : SV_VertexID,
	in uint inInstanceID : SV_InstanceID,
	out float4 outPosition : SV_POSITION,
	out uint outRTIndex : SV_RenderTargetArrayIndex)
{
    outPosition = float4(0.0f, 0.0f, 0.0f, 1.0f);
    outRTIndex = 0;
	
	/*
    ShaderMeshInstancePointer instancePtr = GetMeshInstancePtr(inInstanceID);

	MeshInstance meshInstance = LoadMeshInstance(instancePtr.GetInstanceIndex());
	matrix worldMatrix = meshInstance.WorldMatrix;

	Geometry geometry = LoadGeometry(push.GeometryIndex);
	ByteAddressBuffer vertexBuffer = ResourceHeap_GetBuffer(geometry.VertexBufferIndex);

	uint index = inVertexID;
	float4 position = float4(asfloat(vertexBuffer.Load3(geometry.PositionOffset + index * 12)), 1.0f);

	position = mul(position, worldMatrix);
    outPosition = mul(position, RenderCams.ViewProjection[instancePtr.GetInstanceIndex()]);
    outRTIndex = mul(position, RenderCams.RtIndex[instancePtr.GetInstanceIndex()]);
	
	outRTIndex = inInstanceID;
	*/
}