#ifndef __LIGHTING__HLSLI__
#define __LIGHTING__HLSLI__

#include "Include/Shaders/ShaderInterop.h"
#include "Include/Shaders/ShaderInteropStructures.h"
#include "BRDF.hlsli"

struct LightingPart
{
	float3 Diffuse;
	float3 Specular;

    void Init(float3 diffuse, float3 specular)
    {
        Diffuse = diffuse;
        Specular = specular;
    }
};

struct Lighting
{
	LightingPart Direct;
	LightingPart Indirect;

    void Init()
    {
        Direct.Init(0, 0);
        Indirect.Init(0, 0);
    }
};


float3 ApplyLighting(Lighting lightingTerms, BRDFDataPerSurface brdfSurfaceData, Surface surface)
{
    // input diffuse contains Kd * (Cdiff). Now we finally apply PI to complete Lambert's equation.
    float3 diffuse = (lightingTerms.Direct.Diffuse * brdfSurfaceData.DiffuseReflectance / PI) + (lightingTerms.Indirect.Diffuse * (1 - brdfSurfaceData.F) * surface.AO);
    float3 specular = lightingTerms.Direct.Specular + (lightingTerms.Indirect.Specular * surface.AO); // indirect specular is completed.

    return diffuse + specular + surface.Emissive;
    
}

void ShadeSurface_Direct(
    in ShaderLight light,
    in BRDFDataPerSurface brdfSurfaceData,
    inout LightingPart outDirectRadiance)
{
    float3 lightIncident = 0;
    float lightIrradance = 0;

    // TODO: Revisit lighting model
    const uint lightType = light.GetType();
    if (lightType == ENTITY_TYPE_DIRECTIONALLIGHT)
    {
        lightIncident = normalize(light.GetDirection());
        lightIrradance = light.GetEnergy();
    }
    else if (lightType == ENTITY_TYPE_OMNILIGHT || lightType == ENTITY_TYPE_SPOTLIGHT)
    {
        lightIncident = brdfSurfaceData.P - light.Position;
        // This is the same as squaring the magnature
        const float distance2 = dot(lightIncident, lightIncident);
        const float distance = sqrt(distance2);
        const float range2 = light.GetRange() * light.GetRange();

        if (distance > light.GetRange())
        {
            // return;
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

    BRDFDataPerLight brdfLightData = CreatePerLightBRDFData(lightIncident, brdfSurfaceData);
    
    // Calculate light terms, light colour is applied outside of this functions
    outDirectRadiance.Diffuse = BRDF_DirectDiffuse(brdfSurfaceData, brdfLightData) * lightIrradance;
    outDirectRadiance.Specular = BRDF_DirectSpecular(brdfSurfaceData, brdfLightData) * lightIrradance;
}
#endif // __LIGHTING__HLSLI__