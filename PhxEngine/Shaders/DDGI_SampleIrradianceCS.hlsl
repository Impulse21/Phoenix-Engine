
#include "Globals_NEW.hlsli"
#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"

#include "GBuffer_New.hlsli"

#define THREAD_COUNT 8
#define RS_DDGI_UPDATE \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=16, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "DescriptorTable(SRV(t1, numDescriptors = 6)), " \
    "DescriptorTable( UAV(u0, numDescriptors = 2) )," \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s52, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_LESS_EQUAL),"

PUSH_CONSTANT(push, DDGIPushConstants);
RWTexture2D<float4> OutputIrradianceSample : register(u0);


Texture2D GBuffer_Depth : register(t1);
Texture2D GBuffer_0 : register(t2);
Texture2D GBuffer_1 : register(t3); // Normal Buffer
Texture2D GBuffer_2 : register(t4);
Texture2D GBuffer_3 : register(t5);
Texture2D GBuffer_4 : register(t6);

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

 // Based on: https://github.com/diharaw/hybrid-rendering/blob/master/src/shaders/gi/gi_common.glsl
float3 SampleIrradiance(float3 P, float3 N)
{
    uint3 baseGridCoord = BaseProbeCoord(P);
    float3 baseProbePos = DDGI_ProbeCoordToPosition(baseGridCoord);
    
    float3 sumIrradiance = 0;
    float sumWeight = 0;
    
    // Alpha is how far from the floor(currentVertex) position on [0,1] for each axies
    float3 alpha = saturate((P - baseProbePos) * GetScene().DDGI.CellSizeRcp);

    /*
    // iterate over the adjacent Probe cage
    for (uint i = 0; i < 8; i++)
    {
        // Compute the offset grid coord and clamp to the probe gride boundary
        // offset = 0 or 1 along each axis
        uint3 offset = uint3(i, i >> 1, i >> 2) & 1;
        uint3 probeGridCoord = clamp(baseGridCoord + offset, 0, GetScene().DDGI.GridDimensions);

        // make a cosine falloff in tangent plane with respec to the angle from the surface to the probe so that we never
        // test a prove that is *behind* the surface
        // It doesn't have to be cosine, but that is efficent to compute and we must clip the the tangent plane
        float3 probePos = DDGI_ProbeCoordToPosition(probeGridCoord);
        
        // Bias the position at whcih visibility is computed;
        // this avoids performing a shadow test at a surface, which is a
        // angerous location because that is exactly the line between shawed and unshadowed,
        // If the normal bias is tow small, there will be light and dark leaks. if it's to large,
        // then samples can pass through
        float3 probeToPoint = P - probePos + N * 0.001;
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
        if (GetScene().ddgi.depth_texture >= 0)
        {
			//float2 tex_coord = texture_coord_from_direction(-dir, p, ddgi.depth_texture_width, ddgi.depth_texture_height, ddgi.depth_probe_side_length);
            float2 tex_coord = ddgi_probe_depth_uv(probe_grid_coord, -dir);

            float dist_to_probe = length(probe_to_point);

			//float2 temp = textureLod(depth_texture, tex_coord, 0.0f).rg;
            float2 temp = bindless_textures[GetScene().ddgi.depth_texture].SampleLevel(sampler_linear_clamp, tex_coord, 0).xy;
            float mean = temp.x;
            float variance = abs(sqr(temp.x) - temp.y);

			// http://www.punkuser.net/vsm/vsm_paper.pdf; equation 5
			// Need the max in the denominator because biasing can cause a negative displacement
            float chebyshev_weight = variance / (variance + sqr(max(dist_to_probe - mean, 0.0)));

			// Increase contrast in the weight 
            chebyshev_weight = max(pow(chebyshev_weight, 3), 0.0);

            weight *= (dist_to_probe <= mean) ? 1.0 : chebyshev_weight;
        }
#endif

		// Avoid zero weight
        weight = max(0.000001, weight);

        float3 irradianceDir = N;

		//float2 tex_coord = texture_coord_from_direction(normalize(irradiance_dir), p, ddgi.irradiance_texture_width, ddgi.irradiance_texture_height, ddgi.irradiance_probe_side_length);
        float2 texCoord = ddgi_probe_color_uv(probeGridCoord, irradianceDir);

		//float3 probe_irradiance = textureLod(irradiance_texture, tex_coord, 0.0f).rgb;
        float3 probe_irradiance = bindless_textures[GetScene().ddgi.color_texture].SampleLevel(sampler_linear_clamp, tex_coord, 0).rgb;

		// A tiny bit of light is really visible due to log perception, so
		// crush tiny weights but keep the curve continuous. This must be done
		// before the trilinear weights, because those should be preserved.
        const float crushThreshold = 0.2f;
        if (weight < crushThreshold)
            weight *= weight * weight * (1.0f / SQR(crushThreshold));

		// Trilinear weights
        weight *= trilinear.x * trilinear.y * trilinear.z;

		// Weight in a more-perceptual brightness space instead of radiance space.
		// This softens the transitions between probes with respect to translation.
		// It makes little difference most of the time, but when there are radical transitions
		// between probes this helps soften the ramp.
#ifndef DDGI_LINEAR_BLENDING
        probe_irradiance = sqrt(probe_irradiance);
#endif

        sumIrradiance += weight * probe_irradiance;
        sumWeight += weight;
    }
    */
    return 0;
}

[RootSignature(RS_DDGI_UPDATE)]
[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint2 GTid : SV_GroupThreadID, uint2 GroupId : SV_GroupID)
{
    uint2 pixelPosition = DTid.xy;

    const float depth = GBuffer_Depth.Sample(SamplerDefault, pixelPosition).x;
    
    
    if (depth == 1.0f)
    {
        OutputIrradianceSample[pixelPosition] = 0.0f;

    }
    
    const float3 P = ReconstructWorldPosition(GetCamera(), pixelPosition, depth);
    const float3 N = GBuffer_1[pixelPosition].xyz;
    
    float3 irradiance = push.GiBoost * SampleIrradiance(P, N);

    // Store
    OutputIrradianceSample[pixelPosition] = float4(irradiance, 1.0f);
    // TODO?
}