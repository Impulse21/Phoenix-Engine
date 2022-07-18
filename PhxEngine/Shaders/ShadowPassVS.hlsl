
#include "Globals.hlsli"
#include "Defines.hlsli"

#if USE_RESOURCE_HEAP
#define ShadowPassRS \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=19, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
	RS_BINDLESS_DESCRIPTOR_TABLE \

#else
#define ShadowPassRS \
	"RootConstants(num32BitConstants=19, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
	RS_BINDLESS_DESCRIPTOR_TABLE \

#endif 

PUSH_CONSTANT(push, GeometryPassPushConstants);

struct VertexInput
{
	uint VertexID : SV_VertexID;
	uint InstanceID : SV_InstanceID;
};

[RootSignature(ShadowPassRS)]
float4 main(in VertexInput input) : SV_POSITION
{
	Geometry geometry = LoadGeometry(push.GeometryIndex);
	ByteAddressBuffer vertexBuffer = ResourceHeap_GetBuffer(geometry.VertexBufferIndex);

	uint index = input.VertexID;

	matrix worldMatrix = push.WorldTransform;
	float4 position = float4(asfloat(vertexBuffer.Load3(geometry.PositionOffset + index * 12)), 1.0f);

	position = mul(position, worldMatrix);
	position = mul(position, GetCamera().ViewProjection);

	return position;
}