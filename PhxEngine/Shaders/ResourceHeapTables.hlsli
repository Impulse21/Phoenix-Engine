#ifndef __PHX_RESOURCE_HEAP_TABLES_HLSLI__
#define __PHX_RESOURCE_HEAP_TABLES_HLSLI__


#include "ShaderInterop.h"


#ifndef RS_BINDLESS_DESCRIPTOR_TABLE

#ifndef USE_RESOURCE_HEAP

#define RS_BINDLESS_DESCRIPTOR_TABLE 	"DescriptorTable( " \
		"SRV(t0, space = 100, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 101, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)," \
		"SRV(t0, space = 102, offset = 0, numDescriptors = unbounded, flags = DESCRIPTORS_VOLATILE | DATA_VOLATILE)" \
	"), " 

#define RS_BINDLESS_RS_FLAGS "CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED "

#else

#define RS_BINDLESS_DESCRIPTOR_TABLE ""
#define RS_BINDLESS_RS_FLAGS ""

#endif // USE_RESOURCE_HEAP

#endif // RS_BINDLESS_DESCRIPTOR_TABLE


// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_DynamicResources.html#root-signature-changes
// To use the Dynamic Resource Heap, we need to target SM 6.6 and provide the following to the root signature
// RootFlags( CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | SAMPLER_HEAP_DIRECTLY_INDEXED )
#ifndef USE_RESOURCE_HEAP

Texture2D ResourceHeap_Texture2D[] : register(t0, RESOURCE_HEAP_TEX2D_SPACE);
TextureCube ResourceHeap_TextureCube[] : register(t0, RESOURCE_HEAP_TEX_CUBE_SPACE);
ByteAddressBuffer ResourceHeap_Buffer[] : register(t0, RESOURCE_HEAP_BUFFER_SPACE);

#endif

inline Texture2D ResourceHeap_GetTexture(uint index)
{
#ifdef USE_RESOURCE_HEAP
	return ResourceDescriptorHeap[index];
#else
	return ResourceHeap_Texture2D[index];
#endif
}

inline Texture2D ResourceHeap_GetTexture2D(uint index)
{
#ifdef USE_RESOURCE_HEAP
	return ResourceDescriptorHeap[index];
#else
	return ResourceHeap_Texture2D[index];
#endif
}

inline TextureCube ResourceHeap_GetTextureCube(uint index)
{
#ifdef USE_RESOURCE_HEAP
	return ResourceDescriptorHeap[index];
#else
	return ResourceHeap_TextureCube[index];
#endif
}

inline ByteAddressBuffer ResourceHeap_GetBuffer(uint index)
{
#ifdef USE_RESOURCE_HEAP
	return ResourceDescriptorHeap[index];
#else
	return ResourceHeap_Buffer[index];
#endif
}
#endif