#pragma pack_matrix(row_major)

#define DEFERRED_LIGHTING_COMPILE_PS
#define RT_SHADOWS
// #define USE_HARD_CODED_LIGHT
#include "DeferredLighting.hlsli"