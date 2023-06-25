#ifndef __PHX_DDGI_COMMON_HLSLI__
#define __PHX_DDGI_COMMON_HLSLI__

#include "Globals_NEW.hlsli"
#include "Defines.hlsli"

// DEBUG
// #define DEBUG_RAYS
#ifdef DEBUG_RAYS
static const float3 DEBUG_RAYs[6] =
{
    float3(0, 0, 1),
    float3(0, 0, -1),
    float3(0, 1, 0),
    float3(0, -1, 0),
    float3(1, 0, 0),
    float3(-1, 0, 0)
};
#endif 

// spherical fibonacci: https://github.com/diharaw/hybrid-rendering/blob/master/src/shaders/gi/gi_ray_trace.rgen
#define madfrac(A, B) ((A) * (B)-floor((A) * (B)))
static const float PHI = sqrt(5) * 0.5 + 0.5;
inline float3 SphericalFibonacci(float i, float n)
{
#ifdef DEBUG_RAYS
    return DEBUG_RAYs[i % 6];
#else
    float phi = 2.0 * PI * madfrac(i, PHI - 1);
    float cos_theta = 1.0 - (2.0 * i + 1.0) * (1.0 / n);
    float sin_theta = sqrt(clamp(1.0 - cos_theta * cos_theta, 0.0f, 1.0f));
    return float3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
#endif
}


float2 uvNearest(int2 pixel, float2 textureSize)
{
    float2 uv = floor(pixel) + .5;

    return uv / textureSize;
}
int ProbeIndexFromPixels(int2 pixels, int probeWithBorderSide, int fullTextureWidth)
{
    int probesPerSide = fullTextureWidth / probeWithBorderSide;
    return int(pixels.x / probeWithBorderSide) + probesPerSide * int(pixels.y / probeWithBorderSide);
}
inline uint3 BaseProbeCoord(float3 P)
{
    float3 normalizedPos = (P - GetScene().DDGI.GridStartPosition) * GetScene().DDGI.GridExtentsRcp;
    return floor(normalizedPos * (GetScene().DDGI.GridDimensions - 1));
}

inline uint2 ProbeVisibilityPixel(uint3 probeCoord)
{
    return probeCoord.xz * DDGI_DEPTH_TEXELS + uint2(probeCoord.y * GetScene().DDGI.GridDimensions.x * DDGI_DEPTH_TEXELS, 0) + 1;
}

inline float2 ProbeVisibilityUV(uint3 probeCoord, float3 dir)
{
    float2 pixel = ProbeVisibilityPixel(probeCoord);
    pixel += (EncodeOct(normalize(dir)) * 0.5 + 0.5) * DDGI_DEPTH_RESOLUTION;
    return pixel * GetScene().DDGI.VisibilityTextureResolution.zw;
}
inline uint2 ProbeIrradiancePixel(uint3 probeCoord)
{
    return probeCoord.xz * DDGI_COLOUR_TEXELS + uint2(probeCoord.y * GetScene().DDGI.GridDimensions.x * DDGI_COLOUR_TEXELS, 0) + 1;
}
inline float2 ProbeColourUV(uint3 probeCoord, float3 direction)
{
    float2 pixel = ProbeIrradiancePixel(probeCoord);
    pixel += (EncodeOct(normalize(direction)) * 0.5 + 0.5) * DDGI_COLOUR_RESOLUTION;
    return pixel * GetScene().DDGI.IrradianceTextureResolution.zw;
}

 // Based on: https://github.com/diharaw/hybrid-rendering/blob/master/src/shaders/gi/gi_common.glsl
float3 SampleIrradiance(float3 P, float3 N, float3 cameraPosition)
{
    // TODO: PROMOT TO DEBUG OPTION
    const bool useSmoothBackFace = true;
    const bool usePerceptualEncoding = false;
    
    const float3 Wo = normalize(cameraPosition - P);
    

    const float selfShadowBias = 0.3f;
    const float minDistanceBetweenProbes = 1.0f;
    float3 biasVector = (N * 0.2f + Wo * 0.8f) * (0.75f * minDistanceBetweenProbes) * selfShadowBias;
    float3 biasedWorldPosition = P + biasVector;
    
    uint3 baseGridCoord = BaseProbeCoord(biasedWorldPosition);
    float3 baseProbePos = DDGI_ProbeCoordToPosition(baseGridCoord, false);
    // Alpha is how far from the floor(currentVertex) position on [0,1] for each axies
    float3 alpha = saturate((biasedWorldPosition - baseProbePos) /* * GetScene().DDGI.CellSizeRcp*/);
    
    float3 sumIrradiance = 0;
    float sumWeight = 0;
    
    // iterate over the adjacent Probe cage
    for (uint i = 0; i < 8; i++)
    {
        // Compute the offset grid coord and clamp to the probe gride boundary
        // offset = 0 or 1 along each axis
        uint3 offset = uint3(i, i >> 1, i >> 2) & 1;
        uint3 probeGridCoord = clamp(baseGridCoord + offset, 0, GetScene().DDGI.GridDimensions - 1);
  
        // Make cosine falloff in tangent plane with respect to the angle from the surface to the probe so that we never
        // test a probe that is *behind* the surface.
        // It doesn't have to be cosine, but that is efficient to compute and we must clip to the tangent plane.
        float3 probePos = DDGI_ProbeCoordToPosition(probeGridCoord);
        
        // comute the trilinear weights based on the gride cell vertex to smoothly
        // transition between proves. Avoid ever going entierly to zero because that
        // will cause problems at theborder probes. this isn't really a lerp
        // we're using 1-a when offset = 0 and when offset = 1.
        float3 trilinear = lerp(1.0f - alpha, alpha, offset);
        float weight = 1.0f;
        
		// Clamp all of the multiplies. We can't let the weight go to zero because then it would be 
		// possible for *all* weights to be equally low and get normalized
		// up to 1/n. We want to distinguish between weights that are 
		// low because of different factors.

		// Smooth backface test
        if (useSmoothBackFace)
        {
			// Computed without the biasing applied to the "dir" variable. 
			// This test can cause reflection-map looking errors in the image
			// (stuff looks shiny) if the transition is poor.
            float3 trueDirectionToProbe = normalize(probePos - P);

			// The naive soft backface weight would ignore a probe when
			// it is behind the surface. That's good for walls. But for small details inside of a
			// room, the normals on the details might rule out all of the probes that have mutual
			// visibility to the point. So, we instead use a "wrap shading" test below inspired by
			// NPR work.
			// weight *= max(0.0001, dot(trueDirectionToProbe, wsN));

			// The small offset at the end reduces the "going to zero" impact
			// where this is really close to exactly opposite
            const float dirDotN = (dot(trueDirectionToProbe, N) + 1.0) * 0.5f;
            weight *= SQR(dirDotN) + 0.2;
        }
        
        
        // Bias the position at which visibility is computed; this avoids performing a shadow 
        // test *at* a surface, which is a dangerous location because that is exactly the line
        // between shadowed and unshadowed. If the normal bias is too small, there will be
        // light and dark leaks. If it is too large, then samples can pass through thin occluders to
        // the other side (this can only happen if there are MULTIPLE occluders near each other, a wall surface
        // won't pass through itself.)
        float3 probeToBiasedPointDirection = biasedWorldPosition - probePos;
        float distanceToBiasedPoint = length(probeToBiasedPointDirection);
        probeToBiasedPointDirection *= 1.0 / distanceToBiasedPoint;
        
        // TODO: ADd Flag to enable/disable Visibility Test
		[branch]
        if (GetScene().DDGI.VisibilityAtlasTextureIdPrev >= 0)
        {
			//float2 tex_coord = texture_coord_from_direction(-dir, p, ddgi.depth_texture_width, ddgi.depth_texture_height, ddgi.depth_probe_side_length);
            float2 texCoord = ProbeVisibilityUV(probeGridCoord, probeToBiasedPointDirection);
            float2 visibility = ResourceHeap_GetTexture2D(GetScene().DDGI.VisibilityAtlasTextureIdPrev).SampleLevel(SamplerLinearClamped, texCoord, 0).rg;
            float meanDistanceToOccluder = visibility.x;

            float chebyshevWeight = 1.0;
            if (distanceToBiasedPoint > meanDistanceToOccluder)
            {
                // In "shadow"
                float variance = abs((visibility.x * visibility.x) - visibility.y);
                // http://www.punkuser.net/vsm/vsm_paper.pdf; equation 5
                // Need the max in the denominator because biasing can cause a negative displacement
                const float distance_diff = distanceToBiasedPoint - meanDistanceToOccluder;
                chebyshevWeight = variance / (variance + (distance_diff * distance_diff));
                
                // Increase contrast in the weight
                chebyshevWeight = max((chebyshevWeight * chebyshevWeight * chebyshevWeight), 0.0f);
            }

            // Avoid visibility weights ever going all of the way to zero because when *no* probe has
            // visibility we need some fallback value.
            chebyshevWeight = max(0.05f, chebyshevWeight);
            weight *= chebyshevWeight;
        }

		// Avoid zero weight
        weight = max(0.000001, weight);
        
        // A small amount of light is visible due to logarithmic perception, so
        // crush tiny weights but keep the curve continuous
        const float crushThreshold = 0.2f;
        if (weight < crushThreshold)
        {
            weight *= (weight * weight) * (1.f / (crushThreshold * crushThreshold));
        }
        
        float3 irradianceDir = N;
        float2 texCoord = ProbeColourUV(probeGridCoord, irradianceDir);

		//float3 probe_irradiance = textureLod(irradiance_texture, tex_coord, 0.0f).rgb;
        float3 probeIrradiance = ResourceHeap_GetTexture2D(GetScene().DDGI.IrradianceAtlasTextureIdPrev).SampleLevel(SamplerLinearClamped, texCoord, 0).rgb;
        
        if (usePerceptualEncoding)
        {
            probeIrradiance = pow(probeIrradiance, (0.5f * 5.0f));
        }

		// Trilinear weights
        weight *= trilinear.x * trilinear.y * trilinear.z + 0.001f;
        
        sumIrradiance += weight * probeIrradiance;
        sumWeight += weight;
    }
    

    float3 netIrradiance = sumIrradiance / sumWeight;

    if (usePerceptualEncoding)
    {
        netIrradiance = netIrradiance * netIrradiance;
    }

    float3 irradiance = 0.5f * PI * netIrradiance * 0.95f;

    return irradiance;
}

#endif