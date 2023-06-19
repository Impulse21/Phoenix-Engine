#ifndef __PHX_DDGI_COMMON_HLSLI__
#define __PHX_DDGI_COMMON_HLSLI__

#include "Globals_NEW.hlsli"

float2 uvNearest(int2 pixel, float2 textureSize)
{
    float2 uv = floor(pixel) + .5;

    return uv / textureSize;
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
    return pixel * GetScene().DDGI.VisibilityTextureResolutionRCP;
}
inline uint2 ProbeIrradiancePixel(uint3 probeCoord)
{
    return probeCoord.xz * DDGI_COLOUR_TEXELS + uint2(probeCoord.y * GetScene().DDGI.GridDimensions.x * DDGI_COLOUR_TEXELS, 0) + 1;
}
inline float2 ProbeColourUV(uint3 probeCoord, float3 direction)
{
    float2 pixel = ProbeIrradiancePixel(probeCoord);
    pixel += (EncodeOct(normalize(direction)) * 0.5 + 0.5) * DDGI_COLOUR_RESOLUTION;
    return pixel * GetScene().DDGI.IrradianceTextureResolutionRCP;
}
 // Based on: https://github.com/diharaw/hybrid-rendering/blob/master/src/shaders/gi/gi_common.glsl
float3 SampleIrradiance(float3 P, float3 N, float3 Wo)
{
    // const float3 Wo = normalize(cameraPosition - P);
    uint3 baseGridCoord = BaseProbeCoord(P);
    float3 baseProbePos = DDGI_ProbeCoordToPosition(baseGridCoord);
    
    float3 sumIrradiance = 0;
    float sumWeight = 0;
    
    // Alpha is how far from the floor(currentVertex) position on [0,1] for each axies
    float3 alpha = saturate((P - baseProbePos) * GetScene().DDGI.CellSizeRcp);
    
    // iterate over the adjacent Probe cage
    for (uint i = 0; i < 8; i++)
    {
        // Compute the offset grid coord and clamp to the probe gride boundary
        // offset = 0 or 1 along each axis
        uint3 offset = uint3(i, i >> 1, i >> 2) & 1;
        uint3 probeGridCoord = clamp(baseGridCoord + offset, 0, GetScene().DDGI.GridDimensions - 1);

        // make a cosine falloff in tangent plane with respec to the angle from the surface to the probe so that we never
        // test a prove that is *behind* the surface
        // It doesn't have to be cosine, but that is efficent to compute and we must clip the the tangent plane
        float3 probePos = DDGI_ProbeCoordToPosition(probeGridCoord);
        
        // Bias the position at whcih visibility is computed;
        // this avoids performing a shadow test at a surface, which is a
        // angerous location because that is exactly the line between shawed and unshadowed,
        // If the normal bias is tow small, there will be light and dark leaks. if it's to large,
        // then samples can pass through
        const float NormalBias = 0.25;
        float3 probeToPoint = P - probePos + (N + 3.0 * Wo) * NormalBias;
        // float3 probe_to_point = P - probePos + N * 0.001;
        float3 dir = normalize(-probeToPoint);
        
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
            weight *= lerp(saturate(dot(dir, N)), SQR(max(0.0001, (dot(trueDirectionToProbe, N) + 1.0) * 0.5)) + 0.2, 0);
        }
        
		// Moment visibility test
#if 1
		[branch]
        if (GetScene().DDGI.VisibilityAtlasTextureIdPrev >= 0)
        {
			//float2 tex_coord = texture_coord_from_direction(-dir, p, ddgi.depth_texture_width, ddgi.depth_texture_height, ddgi.depth_probe_side_length);
            float2 texCoord = ProbeVisibilityUV(probeGridCoord, -dir);

            float distToProbe = length(probeToPoint);

			//float2 temp = textureLod(depth_texture, tex_coord, 0.0f).rg;
            float2 temp = ResourceHeap_GetTexture2D(GetScene().DDGI.VisibilityAtlasTextureIdPrev).SampleLevel(SamplerLinearClamped, texCoord, 0).xy;
            float mean = temp.x;
            float variance = abs(SQR(temp.x) - temp.y);

			// http://www.punkuser.net/vsm/vsm_paper.pdf; equation 5
			// Need the max in the denominator because biasing can cause a negative displacement
            float chebyshevWeight = variance / (variance + SQR(max(distToProbe - mean, 0.0)));

			// Increase contrast in the weight 
            chebyshevWeight = max(pow(chebyshevWeight, 3), 0.0);

            weight *= (distToProbe <= mean) ? 1.0 : chebyshevWeight;
        }
#endif

		// Avoid zero weight
        weight = max(0.000001, weight);

        float3 irradianceDir = N;

		//float2 tex_coord = texture_coord_from_direction(normalize(irradiance_dir), p, ddgi.irradiance_texture_width, ddgi.irradiance_texture_height, ddgi.irradiance_probe_side_length);
        float2 texCoord = ProbeColourUV(probeGridCoord, irradianceDir);

		//float3 probe_irradiance = textureLod(irradiance_texture, tex_coord, 0.0f).rgb;
        float3 probeIrradiance = ResourceHeap_GetTexture2D(GetScene().DDGI.IrradianceAtlasTextureIdPrev).SampleLevel(SamplerLinearClamped, texCoord, 0).rgb;

		// A tiny bit of light is really visible due to log perception, so
		// crush tiny weights but keep the curve continuous. This must be done
		// before the trilinear weights, because those should be preserved.
        const float crushThreshold = 0.2f;
        if (weight < crushThreshold)
        {
            weight *= weight * weight * (1.0f / SQR(crushThreshold));
        }

		// Trilinear weights
        weight *= trilinear.x * trilinear.y * trilinear.z;

		// Weight in a more-perceptual brightness space instead of radiance space.
		// This softens the transitions between probes with respect to translation.
		// It makes little difference most of the time, but when there are radical transitions
		// between probes this helps soften the ramp.
#ifndef DDGI_LINEAR_BLENDING
        probeIrradiance = sqrt(probeIrradiance);
#endif

        sumIrradiance += weight * probeIrradiance;
        sumWeight += weight;
    }
    
    if (sumWeight > 0)
    {
        float3 netIrradiance = sumIrradiance / sumWeight;

		// Go back to linear irradiance
#ifndef DDGI_LINEAR_BLENDING
        netIrradiance = SQR(netIrradiance);
#endif

		//net_irradiance *= 0.85; // energy preservation

        return netIrradiance;
		//return 0.5f * PI * net_irradiance;
    }

    return 0;
}

#endif