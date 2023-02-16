
#include "VisibilityBuffer.hlsli"

uint main(PSInput_VisBuffer input, in uint primitiveID : SV_PrimitiveID) : SV_TARGET
{
    VisibilityID index;
    index.InstanceIndex = input.InstanceIndex;
    index.PrimitiveIndex = primitiveID;
    index.SubsetIndex = push.GeometryIndex - LoadMeshInstance(input.InstanceIndex).GeometryOffset;
    
    return index.Pack();
}