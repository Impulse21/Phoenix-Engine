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

void Light_Directional(
    in ShaderLight light,
    in BRDFDataPerSurface brdfSurfaceData,
    inout LightingPart outDirectRadiance)
{
    float3 lightIncident = normalize(light.GetDirection());
    float lightIrradance = light.GetIntensity();

    BRDFDataPerLight brdfLightData = CreatePerLightBRDFData(lightIncident, brdfSurfaceData);

    // No point in doing anything if the light doesn't contribute anything.
    [branch]
    if (any(brdfLightData.NdotL))
    {
        // TODO: Move light colour to here!!
        outDirectRadiance.Diffuse = BRDF_DirectDiffuse(brdfSurfaceData, brdfLightData) * lightIrradance;
        outDirectRadiance.Specular = BRDF_DirectSpecular(brdfSurfaceData, brdfLightData) * lightIrradance;
    }
}

float CalculateAttenuation_Omni(float dist2, float range2)
{
    // GLTF recommendation: https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual#range-property
    // saturate(1 - pow(dist / range, 4)) / dist2;
    
    // Credit Wicked Engine:
    // Removed pow(x, 4), and avoid zero divisions:
    float distPerRange = dist2 / max(0.0001, range2); // pow2
    distPerRange *= distPerRange; // pow4
    return saturate(1 - distPerRange) / max(0.0001, dist2);
}


void Light_Omni(
    in ShaderLight light,
    in BRDFDataPerSurface brdfSurfaceData,
    inout LightingPart outDirectRadiance)
{
    const float3 lightIncident = brdfSurfaceData.P - light.Position;
    const float dist2 = dot(lightIncident, lightIncident);
    const float range = light.GetRange();
    const float range2 = range * range;

    [branch]
    if (dist2 < range2)
    {
        BRDFDataPerLight brdfLightData = CreatePerLightBRDFData(lightIncident, brdfSurfaceData);

        [branch]
        if (any(brdfLightData.NdotL))
        {
            const float attenuation = CalculateAttenuation_Omni(dist2, range2);
            const float lightIrradance = light.GetIntensity() * attenuation;

            // TODO: Move light colour to here!!
            outDirectRadiance.Diffuse = BRDF_DirectDiffuse(brdfSurfaceData, brdfLightData) * lightIrradance;
            outDirectRadiance.Specular = BRDF_DirectSpecular(brdfSurfaceData, brdfLightData) * lightIrradance;
        }
    }

}
float CalculateAttenuation_Spot(
    float spotFactor,
    float angleScale,
    float angleOffset,
    float dist2,
    float range2)
{

    float attenuation = CalculateAttenuation_Omni(dist2, range2);
    float angularAttenuation = saturate(mad(spotFactor, angleScale, angleOffset));
    angularAttenuation *= angularAttenuation;
    attenuation *= angularAttenuation;
    return attenuation;
}


void Light_Spot(
    in ShaderLight light,
    in BRDFDataPerSurface brdfSurfaceData,
    inout LightingPart outDirectRadiance)
{
    const float3 lightIncident = brdfSurfaceData.P - light.Position;
    const float dist2 = dot(lightIncident, lightIncident);
    const float range = light.GetRange();
    const float range2 = range * range;

    [branch]
    if (dist2 < range2)
    {
        BRDFDataPerLight brdfLightData = CreatePerLightBRDFData(lightIncident, brdfSurfaceData);

        [branch]
        if (any(brdfLightData.NdotL))
        {
            const float spotFactor = dot(brdfLightData.L, light.GetDirection());
            const float spotCutoff = light.GetConeAngleCos();

            [branch]
            if (spotFactor > spotCutoff)
            {
                const float attenuation = CalculateAttenuation_Spot(spotFactor, light.GetAngleScale(), light.GetAngleOffset(), dist2, range2);
                const float lightIrradance = light.GetIntensity() * attenuation;

                // TODO: Move light colour to here!!
                outDirectRadiance.Diffuse = BRDF_DirectDiffuse(brdfSurfaceData, brdfLightData) * lightIrradance;
                outDirectRadiance.Specular = BRDF_DirectSpecular(brdfSurfaceData, brdfLightData) * lightIrradance;
            }
        }
    }

}

void ShadeSurface_Direct(
    in ShaderLight light,
    in BRDFDataPerSurface brdfSurfaceData,
    inout LightingPart outDirectRadiance)
{
    float lightIrradance;
    BRDFDataPerLight brdfLightData;
    // TODO: Revisit lighting model
    const uint lightType = light.GetType();
    switch (light.GetType())
    {
    case ENTITY_TYPE_DIRECTIONALLIGHT:
    {
        Light_Directional(light, brdfSurfaceData, outDirectRadiance);
    }
    break;
    case ENTITY_TYPE_OMNILIGHT:
    {
        Light_Omni(light, brdfSurfaceData, outDirectRadiance);
    }
    break;
    case ENTITY_TYPE_SPOTLIGHT:
    {
        Light_Spot(light, brdfSurfaceData, outDirectRadiance);
    }
    break;
    default:
    {
        return;
    }
    }
}



#endif // __LIGHTING__HLSLI__