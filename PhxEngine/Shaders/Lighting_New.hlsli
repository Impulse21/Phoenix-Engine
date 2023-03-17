#ifndef __LIGHTING__HLSLI__
#define __LIGHTING__HLSLI__

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"
#include "BRDF.hlsli"

#include "Shadows.hlsli"

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

void DirectLightContribution(in Scene scene, in BRDFDataPerSurface brdfSurfaceData, in Surface surface, inout Lighting lightingTerms)
{
    // TODO Handle Lights in some way
    float shadow = 1.0;
    float3 lightDirection = normalize(float3(0.70711, -0.70711, 0));
    #ifdef RT_SHADOWS
    CalculateShadowRT(lightDirection, brdfSurfaceData.P, scene.RT_TlasIndex, shadow);
    #endif
    // TODO: Call ShadeSurface_Direct
    BRDFDataPerLight brdfLightData = CreatePerLightBRDFData(lightDirection, brdfSurfaceData);
    
    float LightIntensity = 9.0f;
    float3 directLightContribution = BRDF_DirectDiffuse(brdfSurfaceData, brdfLightData) * 9.0f;
    float3 specularLightContribution = BRDF_DirectSpecular(brdfSurfaceData, brdfLightData) * 9.0f;

    float3 lightColour = 1.0f;
    lightingTerms.Direct.Init(0, 0);
    lightingTerms.Direct.Diffuse += (shadow * directLightContribution) * lightColour;
    lightingTerms.Direct.Specular += (shadow * specularLightContribution) * lightColour;
}

void IndirectLightContribution_IBL(
    in Scene scene,
    in BRDFDataPerSurface brdfSurfaceData,
    in Surface surface,
    TextureCube irradianceMapTex,
    TextureCube prefilteredColourMapTex,
    Texture2D brdfLutTex,
    in SamplerState defaultSampler,
    in SamplerState linearClampedSampler,
    inout Lighting lightingTerms)
{
    // Difuse
    float lodLevel = surface.Roughness * MaxReflectionLod;
    float3 F = FresnelSchlick(brdfSurfaceData.NdotV, brdfSurfaceData.F0, surface.Roughness);
        
    // Improvised abmient lighting by using the Env Irradance map.
    lightingTerms.Indirect.Diffuse = irradianceMapTex.Sample(defaultSampler, brdfSurfaceData.N).rgb;
    
    // Spec
    float3 prefilteredColour = prefilteredColourMapTex.SampleLevel(linearClampedSampler, brdfSurfaceData.R, lodLevel).rgb;
    
    float2 envBrdfUV = float2(brdfSurfaceData.NdotV, surface.Roughness);
    float2 envBrdf = brdfLutTex.Sample(linearClampedSampler, envBrdfUV).rg;

    lightingTerms.Indirect.Specular = prefilteredColour * (brdfSurfaceData.F * envBrdf.x + envBrdf.y);
}

float3 ApplyLighting(Lighting lightingTerms, BRDFDataPerSurface brdfSurfaceData, Surface surface)
{
    // input diffuse contains Kd * (Cdiff). Now we finally apply PI to complete Lambert's equation.
    float3 diffuse = lightingTerms.Direct.Diffuse / PI + lightingTerms.Indirect.Diffuse * (1 - brdfSurfaceData.F) * surface.AO;
    float3 specular = lightingTerms.Direct.Specular + lightingTerms.Indirect.Specular * surface.AO; // indirect specular is completed.

    float3 colour = diffuse * brdfSurfaceData.DiffuseReflectance; // If an object is metallic, this should cancel out the diffuse term.
    colour += specular;
    colour += surface.Emissive;
    return colour;
    
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
        // The Direct Equation: Lo += (kD * albedo / PI + specular) * radiance * NdotL;
        // 
        // LoDiffuse = (kD * albedo / PI) * radiance * NdotL
        //           = ((1 - F) (1 - metalness) * albedo / PI)  * radiance * NdotL
        //  DiffuseReflectance = (1-metalness) * albedo.
        //           = ((1 - F) * DiffuseReflectance / PI)  * radiance * NdotL
        // Moved Diffuse reflectance / PI out to Apply lighting. Consider moving it back for read ability.
        //
        outDirectRadiance.Diffuse = BRDF_DirectDiffuse(brdfSurfaceData, brdfLightData) * lightIrradance;

        // LoSpecular = (Specular) * radiance * NdotL
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
            // The Direct Equation: Lo += (kD * albedo / PI + specular) * radiance * NdotL;
            // 
            // LoDiffuse = (kD * albedo / PI) * radiance * NdotL
            //           = ((1 - F) (1 - metalness) * albedo / PI)  * radiance * NdotL
            //  DiffuseReflectance = (1-metalness) * albedo.
            //           = ((1 - F) * DiffuseReflectance / PI)  * radiance * NdotL
            // Moved Diffuse reflectance / PI out to Apply lighting. Consider moving it back for read ability.
            //
            outDirectRadiance.Diffuse = BRDF_DirectDiffuse(brdfSurfaceData, brdfLightData) * lightIrradance;

            // LoSpecular = (Specular) * radiance * NdotL
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
                // The Direct Equation: Lo += (kD * albedo / PI + specular) * radiance * NdotL;
                // 
                // LoDiffuse = (kD * albedo / PI) * radiance * NdotL
                //           = ((1 - F) (1 - metalness) * albedo / PI)  * radiance * NdotL
                //  DiffuseReflectance = (1-metalness) * albedo.
                //           = ((1 - F) * DiffuseReflectance / PI)  * radiance * NdotL
                // Moved Diffuse reflectance / PI out to Apply lighting. Consider moving it back for read ability.
                //
                outDirectRadiance.Diffuse = BRDF_DirectDiffuse(brdfSurfaceData, brdfLightData) * lightIrradance;

                // LoSpecular = (Specular) * radiance * NdotL
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