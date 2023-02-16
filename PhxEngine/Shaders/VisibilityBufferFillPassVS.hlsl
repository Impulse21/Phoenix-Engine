

#include "../Include/PhxEngine/Shaders/ShaderInteropStructures.h"
#include "Globals.hlsli"
#include "Defines.hlsli"
#include "VertexBuffer.hlsli"
#include "VisibilityBuffer.hlsli"


inline ShaderMeshInstancePointer GetMeshInstancePtr(uint index)
{
    return ResourceHeap_GetBuffer(push.InstancePtrBufferDescriptorIndex).Load<ShaderMeshInstancePointer> (push.InstancePtrDataOffset + index * sizeof(ShaderMeshInstancePointer));
}


[RootSignature(VisibilityBufferFillRS)]
PSInput_VisBuffer main(
	uint vertexID : SV_VertexID,
	uint instanceID : SV_InstanceID)
{
    Geometry geometry = LoadGeometry(push.GeometryIndex);
    float4 position = RetrieveVertexPosition(vertexID, geometry);
    
    ShaderMeshInstancePointer instancePtr = GetMeshInstancePtr(instanceID);
    MeshInstance meshInstance = LoadMeshInstance(instancePtr.GetInstanceIndex());
    meshInstance = ResourceHeap_GetBuffer(GetScene().MeshInstanceBufferIndex).Load<MeshInstance>(instancePtr.GetInstanceIndex() * sizeof(MeshInstance));
    
    PSInput_VisBuffer output;
    output.Position = mul(position, meshInstance.WorldMatrix);
    output.Position = mul(output.Position, GetCamera().ViewProjection);
    
    return output;
}