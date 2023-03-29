#ifndef __SHADOWS_HLSL__
#define __SHADOWS_HLSL__

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"


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
