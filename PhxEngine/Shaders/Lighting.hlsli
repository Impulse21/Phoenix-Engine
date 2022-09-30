#ifndef __LIGHTING__HLSLI__
#define __LIGHTING__HLSLI__

#include "Include/Shaders/ShaderInterop.h"
#include "Include/Shaders/ShaderInteropStructures.h"
#include "BRDFFunctions.hlsli"

struct LightingPart
{
	float3 Diffuse;
	float3 Specular;
};

struct Lighting
{
	LightingPart Direct;
	LightingPart Indirect;
};


float3 ApplyLighting(Lighting lightingTerms)
{
    return lightingTerms.Direct.Diffuse 
        + lightingTerms.Direct.Specular 
        + lightingTerms.Indirect.Diffuse 
        + lightingTerms.Indirect.Specular;
}
SurfaceProperties DefaultSurfaceProperties()
{
    SurfaceProperties result;

    result.Albedo = 0;
    result.Opactiy = 1;
    result.Normal = 0;
    result.Roughness = 0;
    result.Metalness = 0;
    result.AO = 0;

    return result;
}

void ShadeSurface_Direct(
    in ShaderLight light,
    in SurfaceProperties surfaceProperties,
    in float3 surfacePosition,
    in float3 viewIncident,
    inout LightingPart outDirectRadiance)
{
    float3 lightIncident = 0;
    float lightIrradance = 0;

    const uint lightType = light.GetType();
    if (lightType == ENTITY_TYPE_DIRECTIONALLIGHT)
    {
        lightIncident = normalize(light.GetDirection());
        lightIrradance = light.GetEnergy();
    }
    else if (lightType == ENTITY_TYPE_OMNILIGHT || lightType == ENTITY_TYPE_SPOTLIGHT)
    {
        const float3 lightIncident = surfacePosition - light.Position;
        // This is the same as squaring the magnature
        const float distance2 = dot(lightIncident, lightIncident);
        const float distance = sqrt(distance2);
        const float range2 = light.GetRange() * light.GetRange();

        if (distance > light.GetRange())
        {
            return;
        }

        // Calculate attenutaion
        const float att = saturate(1 - (distance2 / range2));
        float attenuation = att * att;

        if (lightType == ENTITY_TYPE_SPOTLIGHT)
        {
            const float spotFactor = dot(lightIncident, light.GetDirection());
            const float spotCutoff = light.GetConeAngleCos();
            attenuation *= saturate((1 - (1 - spotFactor) * 1 / (1 - spotCutoff)));
        }

        lightIrradance = light.GetEnergy() * attenuation;
    }
    else
    {
        // Unknown light type...
        return;
    }

    BrdfData brdfData = PrepareBrdfData(viewIncident, lightIncident, surfaceProperties);
    
    // Calculate light terms, light colour is applied outside of this functions
    outDirectRadiance.Diffuse = LambertDiffuse(brdfData) * lightIrradance;
    outDirectRadiance.Specular = CookTorranceSpecular(brdfData) * lightIrradance;
}
#endif // __LIGHTING__HLSLI__