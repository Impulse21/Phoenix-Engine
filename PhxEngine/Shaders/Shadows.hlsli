#ifndef __SHADOWS_HLSL__
#define __SHADOWS_HLSL__

#include "ShaderInteropStructures.h"


inline float3 sample_shadow(float2 uv, float cmp)
{
	[branch]
	if (GetFrame().texture_shadowatlas_index < 0)
		return 0;

	Texture2D texture_shadowatlas = bindless_textures[GetFrame().texture_shadowatlas_index];
	float3 shadow = texture_shadowatlas.SampleCmpLevelZero(sampler_cmp_depth, uv, cmp).r;

	return shadow;
}

// Shout out to Wicked Engine here again.
// Convert texture coordinates on a cubemap face to cubemap sampling coordinates:
// direction	: direction that is usable for cubemap sampling
// returns float3 that has uv in .xy components, and face index in Z component
//	https://stackoverflow.com/questions/53115467/how-to-implement-texturecube-using-6-sampler2d
inline float3 CubemapToUV(in float3 r)
{
	float faceIndex = 0;
	float3 absr = abs(r);
	float3 uvw = 0;
	if (absr.x > absr.y && absr.x > absr.z)
	{
		// x major
		float negx = step(r.x, 0.0);
		uvw = float3(r.zy, absr.x) * float3(lerp(-1.0, 1.0, negx), -1, 1);
		faceIndex = negx;
	}
	else if (absr.y > absr.z)
	{
		// y major
		float negy = step(r.y, 0.0);
		uvw = float3(r.xz, absr.y) * float3(1.0, lerp(1.0, -1.0, negy), 1.0);
		faceIndex = 2.0 + negy;
	}
	else
	{
		// z major
		float negz = step(r.z, 0.0);
		uvw = float3(r.xy, absr.z) * float3(lerp(1.0, -1.0, negz), -1, 1);
		faceIndex = 4.0 + negz;
	}
	return float3((uvw.xy / uvw.z + 1) * 0.5, faceIndex);
}

inline float ShadowPointLight(ShaderLight light, float3 LUnnormalized)
{
	const float remappedDistance = light.GetCubemapDepthRemapNear() + light.GetCubemapDepthRemapFar() / (max(max(abs(LUnnormalized.x), abs(LUnnormalized.y)), abs(LUnnormalized.z)) * 0.989); // little bias to avoid artifact
	const float3 UVSlice = CubemapToUV(-LUnnormalized);
	float2 shadowUV = UVSlice.xy;
	shadow_border_shrink(light, shadowUV);
	shadowUV.x += UVSlice.z;
	shadowUV = mad(shadowUV, light.ShadowAtlasMulAdd.xy, light.ShadowAtlasMulAdd.zw);
	return SampleShadow(shadowUV, remappedDistance);
}
#endif 
