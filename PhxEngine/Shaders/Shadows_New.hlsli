#ifndef __SHADOWS_HLSL__
#define __SHADOWS_HLSL__

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"
#include "Defines.hlsli"
#include "Globals_New.hlsli"

#define DISABLE_TRANSPARENT_SHADOWMAP
// Credit: Wicked Engine  https://wickedengine.net/
inline float3 sampleShadow(float2 uv, float cmp)
{
	[branch]
    if (GetFrame().ShadowAtlasIdx == InvalidDescriptorIndex)
    {
        return 0;
    }

    Texture2D textureShadowAtlas = ResourceHeap_GetTexture2D(GetFrame().ShadowAtlasIdx);
    float3 shadow = textureShadowAtlas.SampleCmpLevelZero(ShadowSampler, uv, cmp).r;

#ifndef DISABLE_SOFT_SHADOWMAP
	// sample along a rectangle pattern around center:
    shadow.x += textureShadowAtlas.SampleCmpLevelZero(ShadowSampler, uv, cmp, int2(-1, -1)).r;
    shadow.x += textureShadowAtlas.SampleCmpLevelZero(ShadowSampler, uv, cmp, int2(-1, 0)).r;
    shadow.x += textureShadowAtlas.SampleCmpLevelZero(ShadowSampler, uv, cmp, int2(-1, 1)).r;
    shadow.x += textureShadowAtlas.SampleCmpLevelZero(ShadowSampler, uv, cmp, int2(0, -1)).r;
    shadow.x += textureShadowAtlas.SampleCmpLevelZero(ShadowSampler, uv, cmp, int2(0, 1)).r;
    shadow.x += textureShadowAtlas.SampleCmpLevelZero(ShadowSampler, uv, cmp, int2(1, -1)).r;
    shadow.x += textureShadowAtlas.SampleCmpLevelZero(ShadowSampler, uv, cmp, int2(1, 0)).r;
    shadow.x += textureShadowAtlas.SampleCmpLevelZero(ShadowSampler, uv, cmp, int2(1, 1)).r;
    shadow = shadow.xxx / 9.0;
#endif // DISABLE_SOFT_SHADOWMAP

#ifndef DISABLE_TRANSPARENT_SHADOWMAP
	[branch]
    if (GetFrame().options & OPTION_BIT_TRANSPARENTSHADOWS_ENABLED && GetFrame().texture_shadowatlas_transparent_index)
    {
        Texture2D texture_shadowatlas_transparent = bindless_textures[GetFrame().texture_shadowatlas_transparent_index];
        float4 transparent_shadow = texture_shadowatlas_transparent.SampleLevel(sampler_linear_clamp, uv, 0);
#ifdef TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
		if (transparent_shadow.a > cmp)
#endif // TRANSPARENT_SHADOWMAP_SECONDARY_DEPTH_CHECK
		{
            shadow *= transparent_shadow.rgb;
        }
    }
#endif //DISABLE_TRANSPARENT_SHADOWMAP

    return shadow;
}


// This is used to pull the uvs to the center to avoid sampling on the border and overfiltering into a different shadow
// Credit: Wicked Engine  https://wickedengine.net/
inline void shadowBorderShrink(in Light light, inout float2 shadowUV)
{
    const float2 shadowResolution = light.ShadowAtlasMulAdd.xy * GetFrame().ShadowAtlasRes;
#ifdef DISABLE_SOFT_SHADOWMAP
	const float border_size = 0.5;
#else
    const float border_size = 1.5;
#endif // DISABLE_SOFT_SHADOWMAP
    shadowUV *= shadowResolution - border_size * 2;
    shadowUV += border_size;
    shadowUV /= shadowResolution;
}

// Credit: Wicked Engine  https://wickedengine.net/
inline float ShadowCube(in Light light, float3 Lunnormalized)
{
    const float remappedDistance = light.GetCubeNearZ() + light.GetCubeFarZ() / (max(max(abs(Lunnormalized.x), abs(Lunnormalized.y)), abs(Lunnormalized.z)) * 0.989); // little bias to avoid artifact
    const float3 uvSlice = CubemapToUv(-Lunnormalized);
    float2 shadowUV = uvSlice.xy;
    shadowBorderShrink(light, shadowUV);
    shadowUV.x += uvSlice.z;
    shadowUV = mad(shadowUV, light.ShadowAtlasMulAdd.xy, light.ShadowAtlasMulAdd.zw);
    return sampleShadow(shadowUV, remappedDistance);
}

#ifdef RT_SHADOWS

inline void CalculateShadowRT(float3 lightDir, float3 surfacePosition, uint tlasIndex, inout float shadow)
{
	if (tlasIndex >= 0)
	{
		RayDesc ray;
		ray.Origin = surfacePosition;
		ray.Direction = -lightDir;
		ray.TMin = 0.001;
		ray.TMax = 1000;

		RayQuery<RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER> rayQuery;

		rayQuery.TraceRayInline(
			ResourceHeap_GetRTAccelStructure(tlasIndex),
			RAY_FLAG_NONE, // OR'd with flags above
			0xff,
			ray);

		rayQuery.Proceed();

		if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
		{
			shadow = 0.0f;
		}

		// From Donut Engine
		/*
		while (rayQuery.Proceed())
		{
			if (rayQuery.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
			{
				if (considerTransparentMaterial(
					rayQuery.CandidateInstanceID(),
					rayQuery.CandidatePrimitiveIndex(),
					rayQuery.CandidateGeometryIndex(),
					rayQuery.CandidateTriangleBarycentrics()))
				{
					rayQuery.CommitNonOpaqueTriangleHit();
				}
			}
		}
		*/

	}
}

#endif 
#endif 
