#pragma pack_matrix(row_major)

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"

#include "Globals_New.hlsli"
#include "VertexFetch.hlsli"
#include "Defines.hlsli"

#define ShadowPassRS \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=1, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "CBV(b2),"  \

PUSH_CONSTANT(push, GeometryPushConstant);


// TODO: Consider switching this so we bind a global buffer.
ConstantBuffer<ShadowCams> SCam : register(b2);

[RootSignature(ShadowPassRS)]
void main(
	in uint inVertexID : SV_VertexID,
	in uint inInstanceID : SV_InstanceID,
	out float4 outPosition : SV_POSITION,
	out uint outViewportId : SV_ViewportArrayIndex)
{
    ObjectInstance objectInstance = LoadObjectInstnace(push.DrawId >> 16);
    Geometry geometryData = LoadGeometry(objectInstance.GeometryIndex);
	
    VertexData vertexData = FetchVertexData(inVertexID, geometryData);
    float4x4 worldMatrix = objectInstance.WorldMatrix;
	// TODO: Set Light Projections
    float4x4 lightViewProj = SCam.ViewProjection[inInstanceID];
	
    outPosition = mul(vertexData.Position, worldMatrix);
    outPosition = mul(outPosition, lightViewProj);
    outViewportId = inInstanceID;
}