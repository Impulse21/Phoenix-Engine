#ifndef __SHADOWS_HLSL__
#define __SHADOWS_HLSL__

#include "Include/Shaders/ShaderInterop.h"


inline void CalculateDirectionalShadow(ShaderLight light, float3 surfacePosition, inout float shadow)
{
	// [loop]
	for (int cascade = 0; cascade < light.GetNumCascades(); cascade++)
	{
		[branch]
		if (light.CascadeTextureIndex >= 0)
		{
			float4x4 shadowMatrix = LoadMatrix(light.GetIndices() + cascade);
			float3 shadowPos = mul(float4(surfacePosition, 1.0f), shadowMatrix).xyz;
			float3 shadowUV = ClipSpaceToUV(shadowPos);
			const float3 cascadeEdgeFactor = saturate(saturate(abs(shadowPos)) - 0.8) * 5.0; // fade will be on edge and inwards 20%
			const float cascadeFade = max(cascadeEdgeFactor.x, max(cascadeEdgeFactor.y, cascadeEdgeFactor.z));

			Texture2DArray cascadeTextureArray = ResourceHeap_GetTexture2DArray(light.CascadeTextureIndex);
			const float shadowMain = cascadeTextureArray.SampleCmpLevelZero(
				ShadowSampler,
				float3(shadowUV.xy, cascade),
				shadowPos.z).r;

			// If we are on cascade edge threshold and not the last cascade, then fallback to a larger cascade:
			[branch]
			if (cascadeFade > 0 && cascade < light.GetNumCascades() - 1)
			{
				// Project into next shadow cascade (no need to divide by .w because ortho projection!):
				cascade += 1;

				shadowMatrix = LoadMatrix(light.GetIndices() + cascade);
				shadowPos = mul(float4(surfacePosition, 1.0f), shadowMatrix).xyz;
				shadowUV = ClipSpaceToUV(shadowPos);
				const float shadowFallback = cascadeTextureArray.SampleCmpLevelZero(
					ShadowSampler,
					float3(shadowUV.xy, cascade),
					shadowPos.z).r;

				shadow *= lerp(shadowMain, shadowFallback, cascadeFade);
			}
			else
			{
				shadow *= shadowMain;
			}
			break;
		}
	}
}


#ifdef RT_SHADOWS

inline void  CalculateShadowRT(ShaderLight light, float3 surfacePosition, uint tlasIndex, inout float shadow)
{
	RayDesc ray;
	ray.Origin = surfacePosition;
	ray.Direction = -light.GetDirection();
	ray.TMin = 0;
	ray.TMax = 1000;

	RayQuery<RAY_FLAG_CULL_NON_OPAQUE |
		RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> rayQuery;

	rayQuery.TraceRayInline(
		ResourceHeap_GetRTAccelStructure(tlasIndex),
		RAY_FLAG_NONE, // OR'd with flags above
		0xff,
		ray);

	rayQuery.Proceed();

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

	if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
	{
		shadow = 0.0f;
	}

}
#endif 
#endif 
