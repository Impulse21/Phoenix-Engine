#pragma pack_matrix(row_major)


#include "DDGI_Common.hlsli"
#include "Defines.hlsli"
#include "VertexFetch.hlsli"
#include "BRDF.hlsli"
#include "Lighting_New.hlsli"
#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"

#define RS_DDGI_RT \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"RootConstants(num32BitConstants=20, b999), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "DescriptorTable( UAV(u0, numDescriptors = 2) )," \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s52, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_LESS_EQUAL),"

RWTexture2D<float4> RadianceOutput : register(u0);
RWTexture2D<float4> DistanceDirectionOutput : register(u1);

PUSH_CONSTANT(push, DDGIPushConstants);

// spherical fibonacci: https://github.com/diharaw/hybrid-rendering/blob/master/src/shaders/gi/gi_ray_trace.rgen
#define madfrac(A, B) ((A) * (B)-floor((A) * (B)))
static const float PHI = sqrt(5) * 0.5 + 0.5;
float3 SphericalFibonacci(float i, float n)
{
    float phi = 2.0 * PI * madfrac(i, PHI - 1);
    float cos_theta = 1.0 - (2.0 * i + 1.0) * (1.0 / n);
    float sin_theta = sqrt(clamp(1.0 - cos_theta * cos_theta, 0.0f, 1.0f));
    return float3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}

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
[RootSignature(RS_DDGI_RT)]
[numthreads(THREADS_PER_WAVE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    const uint probeIndex = Gid.x;
    const uint3 probeCoord = DDGI_GetProbeCoord(probeIndex);
    const float3 probePos = DDGI_ProbeCoordToPosition(probeCoord);
    
    const uint rayCount = push.NumRays;
    const float3x3 randomRotation = (float3x3) push.RandRotation;
    for (uint rayIndex = groupIndex; rayIndex < rayCount; rayIndex += THREADS_PER_WAVE)
    {
        float3 radiance = 0;
        float depth = 0;
        RayDesc ray;
        ray.Origin = probePos;
        ray.TMin = 0; // Not tracing from a surface, so this can be zero since there is no concern with self hits
        ray.TMax = FLT_MAX;
        
        // Select a random location along the spherical Fibonacci and a random orientation matrix.
        ray.Direction = normalize(mul(randomRotation, SphericalFibonacci(rayIndex, push.NumRays)));
#ifdef DEBUG_RAYS
        ray.Direction = DEBUG_RAYs[rayIndex];
#endif
        
        
        RayQuery < RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_FORCE_OPAQUE > rayQuery;
        
        rayQuery.TraceRayInline(
			ResourceHeap_GetRTAccelStructure(GetScene().RT_TlasIndex),
			RAY_FLAG_NONE, // OR'd with flags above
			0xff, // Include all instances
			ray);
        
        while (rayQuery.Proceed());
        if (rayQuery.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
        {
            // radiance = float3(0.529, 0.807, 0.921);
            radiance = 0;
            depth = 1000.0f;
        }
        else
        {
            depth = rayQuery.CommittedRayT();
            // We hit a surface
            if (!rayQuery.CommittedTriangleFrontFace())
            {
                depth *= -0.2;
            }
            else
            {
                ray.Origin = rayQuery.WorldRayOrigin() + rayQuery.WorldRayDirection() * rayQuery.CommittedRayT();
                const uint instanceIndex = rayQuery.CommittedInstanceID();
                // Load the instance data
            
                ObjectInstance objectInstance = LoadObjectInstnace(instanceIndex);
                Geometry geometryData = LoadGeometry(objectInstance.GeometryIndex);
            
                // Get start Index of the primitive in the global index buffer
                const uint primitiveIndex = rayQuery.CommittedPrimitiveIndex();
                const uint startIndex = primitiveIndex * 3 + geometryData.IndexOffset;
            
                StructuredBuffer<uint> indexBuffer = ResourceDescriptorHeap[GetScene().GlobalIndexBufferIdx];
                uint i0 = indexBuffer[startIndex + 0];
                uint i1 = indexBuffer[startIndex + 1];
                uint i2 = indexBuffer[startIndex + 2];
            
                // Get Position Data
                VertexData v0 = FetchVertexData(i0, geometryData);
                VertexData v1 = FetchVertexData(i1, geometryData);
                VertexData v2 = FetchVertexData(i2, geometryData);
            
                // Now we can calculate the position using the baryCentric weights
                // https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#rayquery-committedinstanceid
                const float2 barycentricWeights = rayQuery.CommittedTriangleBarycentrics();
                float2 uv = BarycentricInterpolation(v0.TexCoord, v1.TexCoord, v2.TexCoord, barycentricWeights);
            
                // Sample Albedo for colour info
            
                Material material = LoadMaterial(geometryData.MaterialIndex);
                Surface surface = DefaultSurface();
                surface.Albedo = 1.0f;
                surface.Albedo *= material.AlbedoColour * UnpackRGBA(objectInstance.Colour).rgb;
                if (material.AlbedoTexture != InvalidDescriptorIndex)
                {
                // Sample a LOD version for better performance
                    surface.Albedo *= ResourceHeap_GetTexture2D(material.AlbedoTexture).Sample(SamplerDefault, uv).xyz;
                }
    
                surface.Emissive = UnpackRGBA(material.EmissiveColourPacked).rgb * UnpackRGBA(objectInstance.Emissive).rgb;
            
                // Since we are not storing much info, just use the surface normal for now and see how things look
                float3 normal = BarycentricInterpolation(v0.Normal, v1.Normal, v2.Normal, barycentricWeights);
                surface.Normal = mul(normal, (float3x3) objectInstance.WorldMatrix).xyz;
            
                /*
                float3 positionWS = BarycentricInterpolation(v0.Position, v1.Position, v2.Position, barycentricWeights);
                positionWS = mul(float4(positionWS, 1.0f), objectInstance.WorldMatrix);
                */
            
                const float3 P = ray.Origin;
                // const float3 L = -ray.Direction;
            
                float3 hitResult = 0.0f;
                 [loop]
                for (int iLight = 0; iLight < GetScene().LightCount; iLight++)
                {
                    Light light = LoadLight(iLight);
                    float3 L = 0.0f;
                    float dist = 0.0f;
                    float NdotL = 0.0f;
                
                    Lighting lighting;
                    lighting.Init();
                    const uint lightType = light.GetType();
                    switch (lightType)
                    {
                        case LIGHT_TYPE_DIRECTIONAL:
                        {
                                dist = FLT_MAX;
                                L = normalize(light.GetDirection());
                                NdotL = saturate(dot(L, surface.Normal));
                        
                            [branch]
                                if (NdotL > 0)
                                {
                                    lighting.Direct.Diffuse = light.GetColour().rgb;
                                }
                            }
                            break;
                        case LIGHT_TYPE_OMNI:
                        {
                                L = light.Position - P;
                    
                                const float dist2 = dot(L, L);
                                const float range = light.GetRange();
                                const float range2 = range * range;
                    
                            [branch]
                                if (dist2 < range2)
                                {
                                    dist = sqrt(dist2);
                                    L /= dist;
                                    NdotL = saturate(dot(L, surface.Normal));
                        
                                [branch]
                                    if (NdotL > 0)
                                    {
                                        lighting.Direct.Diffuse = (light.GetColour() * AttenuationOmni(dist2, range2)).rgb;
                                    }
                                }
                            }
                            break;
                    
                        case LIGHT_TYPE_SPOT:
                        {
                                L = light.Position - P;
                    
                                const float dist2 = dot(L, L);
                                const float range = light.GetRange();
                                const float range2 = range * range;
                    
                            [branch]
                                if (dist2 < range2)
                                {
                                    dist = sqrt(dist2);
                                    L /= dist;
                                    NdotL = saturate(dot(L, surface.Normal));
                        
                                [branch]
                                    if (NdotL > 0)
                                    {
                                        const float spotFactor = dot(L, light.GetDirection());
                                        const float spotCutoff = light.GetConeAngleCos();
                                    
                                    [branch]
                                        if (spotFactor > spotCutoff)
                                        {
                                            const float3 lightColor = light.GetColour().rgb;

                                            lighting.Direct.Diffuse = (light.GetColour() * AttenuationSpot(dist2, range2, spotFactor, light.GetAngleScale(), light.GetAngleOffset())).rgb;
                                        }
                                    }
                                }
                            }
                            break;
                        default:
                            break;
                    }
                
                    if (NdotL > 0 && dist > 0)
                    {
                        float3 shadow = 1.0f;
                        RayDesc shadowRay;
                        shadowRay.Origin = P;
                        shadowRay.Direction = L;
                        shadowRay.TMin = 0.001;
                        shadowRay.TMax = dist;
                    
                        rayQuery.TraceRayInline(
						ResourceHeap_GetRTAccelStructure(GetScene().RT_TlasIndex),
						RAY_FLAG_CULL_FRONT_FACING_TRIANGLES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, // uint RayFlags
						0xFF, // uint InstanceInclusionMask
						shadowRay // RayDesc Ray
					);
                        while (rayQuery.Proceed());
                        if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
                        {
                            shadow = 0.0f;
                        }
                    
                        if (any(shadow))
                        {
                            hitResult += max(0, shadow * lighting.Direct.Diffuse * NdotL / PI);
                        }
                    }
                }
            
            	// Infinite bounces based on previous frame probe sampling:
                if (GetScene().DDGI.FrameIndex > 0)
                {
                    const float energyConservation = 0.95;
                    const float3 Wo = -ray.Direction;
                    hitResult += SampleIrradiance(P, surface.Normal, Wo) * energyConservation;
                }
                
                hitResult *= surface.Albedo;
                hitResult += surface.Emissive;
                
                radiance = hitResult;
            }
            
        }
        
        RadianceOutput[float2(rayIndex, probeIndex)] = float4(radiance.rgb, depth);
        DistanceDirectionOutput[float2(rayIndex, probeIndex)] = float4(ray.Direction.xyz, depth);
    }
}