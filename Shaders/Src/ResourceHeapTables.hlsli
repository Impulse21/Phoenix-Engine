#ifndef __PHX_RESOURCE_HEAP_TABLES_HLSLI__
#define __PHX_RESOURCE_HEAP_TABLES_HLSLI__


#include "../Include/Shaders/ShaderInterop.h"

Texture2D ResourceHeap_Texture2D[] : register(t0, RESOURCE_HEAP_TEX2D_SPACE);
TextureCube ResourceHeap_TextureCube[] : register(t0, RESOURCE_HEAP_TEX_CUBE_SPACE);
ByteAddressBuffer ResourceHeap_Buffer[] : register(t0, RESOURCE_HEAP_BUFFER_SPACE);

#endif