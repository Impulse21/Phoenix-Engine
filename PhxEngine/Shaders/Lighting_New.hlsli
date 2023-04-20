#ifndef __LIGHTING__HLSLI__
#define __LIGHTING__HLSLI__

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "../Include/PhxEngine/Shaders/ShaderInteropStructures_New.h"
#include "Globals_New.hlsli"
#include "BRDF.hlsli"

#include "Shadows_New.hlsli"


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


inline void ApplyDirectionalLight(in Light light, in BRDFDataPerSurface brdfSurfaceData, inout Lighting lighting)
{
    const float3 L = normalize(light.GetDirection());
    BRDFDataPerLight brdfLightData = CreatePerLightBRDFData(L, brdfSurfaceData);
    
    // No point in doing anything if the light doesn't contribute anything.
    [branch]
    if (any(brdfLightData.NdotL))
    {
        float3 shadow = 1.0f;
        
        // If completely in shadow, nothing more to do.
        if (any(shadow))
        {
            const float3 lightColour = light.GetColour().rgb * shadow;

            // Calculate lighting
            lighting.Direct.Diffuse = mad(lightColour, BRDF_DirectDiffuse(brdfSurfaceData, brdfLightData), lighting.Direct.Diffuse);
            lighting.Direct.Specular = mad(lightColour, BRDF_DirectSpecular(brdfSurfaceData, brdfLightData), lighting.Direct.Specular);
        }
    }
}

inline float AttenuationOmni(in float dist2, in float range2)
{
    // GLTF recommendation: https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual#range-property
    // saturate(1 - pow(dist / range, 4)) / dist2;
    
    // Credit Wicked Engine: https://wickedengine.net/
    // Removed pow(x, 4), and avoid zero divisions:
    float distPerRange = dist2 / max(0.0001, range2); // pow2
    distPerRange *= distPerRange; // pow4
    return saturate(1 - distPerRange) / max(0.0001, dist2);
}

inline void ApplyOmniLight(in Light light, in BRDFDataPerSurface brdfSurfaceData, inout Lighting lighting)
{
    float3 L = brdfSurfaceData.P - light.Position;
    const float dist2 = dot(L, L);
    const float range = light.GetRange();
    const float range2 = range * range;
    
    [branch]
    if (dist2 < range2)
    {
        const float3 Lunnormalized = L;
        L = normalize(L);
        BRDFDataPerLight brdfLightData = CreatePerLightBRDFData(L, brdfSurfaceData);
        
        [branch]
        if (any(brdfLightData.NdotL))
        {
            float3 shadow = 1.0f;
            
            if (light.IsCastingShadows())
            {
            // Calculate Shadow
            [branch]
                if (GetFrame().Flags & FRAME_FLAGS_DISABLE_CULL_FRUSTUM)
                {
#ifdef RT_SHADOWS
                CalculateShadowRT(lightDirection, brdfSurfaceData.P, scene.RT_TlasIndex, shadow);
#endif
                }
                else
                {
                    shadow *= ShadowCube(light, Lunnormalized);
                }
            }
            
            // If completely in shadow, nothing more to do.
            if (any(shadow))
            {
                float3 lightColour = light.GetColour().rgb * shadow;
                lightColour *= AttenuationOmni(dist2, range2);
                
                // Calculate lighting
                lighting.Direct.Diffuse = mad(lightColour, BRDF_DirectDiffuse(brdfSurfaceData, brdfLightData), lighting.Direct.Diffuse);
                lighting.Direct.Specular = mad(lightColour, BRDF_DirectSpecular(brdfSurfaceData, brdfLightData), lighting.Direct.Specular);
            }
        }
    }
}

float AttenuationSpot(in float dist2, in float range2, in float spotFactor, in float angleScale, in float angleOffset)
{

    float attenuation = AttenuationOmni(dist2, range2);
    float angularAttenuation = saturate(mad(spotFactor, angleScale, angleOffset));
    angularAttenuation *= angularAttenuation;
    attenuation *= angularAttenuation;
    return attenuation;
}

inline void ApplySpotLight(in Light light, in BRDFDataPerSurface brdfSurfaceData, inout Lighting lighting)
{
    float3 L = brdfSurfaceData.P - light.Position;
    const float dist2 = dot(L, L);
    const float range = light.GetRange();
    const float range2 = range * range;
    
    [branch]
    if (dist2 < range2)
    {
        L = normalize(L);
        BRDFDataPerLight brdfLightData = CreatePerLightBRDFData(L, brdfSurfaceData);
        
        [branch]
        if (any(brdfLightData.NdotL))
        {
            const float spotFactor = dot(L, light.GetDirection());
            const float spotCutoff = light.GetConeAngleCos();
            [branch]
            if (spotFactor > spotCutoff)
            {
                float3 shadow = 1.0f;
            
                // If completely in shadow, nothing more to do.
                if (any(shadow))
                {
                    float3 lightColour = light.GetColour().rgb * shadow;
                    lightColour *= AttenuationSpot(dist2, range2, spotFactor, light.GetAngleScale(), light.GetAngleOffset());
                
                    // Calculate lighting
                    lighting.Direct.Diffuse = mad(lightColour, BRDF_DirectDiffuse(brdfSurfaceData, brdfLightData), lighting.Direct.Diffuse);
                    lighting.Direct.Specular = mad(lightColour, BRDF_DirectSpecular(brdfSurfaceData, brdfLightData), lighting.Direct.Specular);
                }
            }
        }
    }
}
void ProcessLight(in Light light, in BRDFDataPerSurface brdfSurfaceData, inout Lighting lightingTerms)
{
    const uint lightType = light.GetType();
    switch (lightType)
    {
        case LIGHT_TYPE_DIRECTIONAL:
            ApplyDirectionalLight(light, brdfSurfaceData, lightingTerms);
            break;
        case LIGHT_TYPE_OMNI:
            ApplyOmniLight(light, brdfSurfaceData, lightingTerms);
            break;
        case LIGHT_TYPE_SPOT:
            ApplySpotLight(light, brdfSurfaceData, lightingTerms);
            break;
        default:
            break;
    }
}

void DirectLightContribution_Simple(in Scene scene, in BRDFDataPerSurface brdfSurfaceData, in Surface surface, inout Lighting lightingTerms)
{
    
    lightingTerms.Direct.Init(0, 0);
    
    #ifdef USE_HARD_CODED_LIGHT
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
    lightingTerms.Direct.Diffuse += (shadow * directLightContribution) * lightColour;
    lightingTerms.Direct.Specular += (shadow * specularLightContribution) * lightColour;
#else
    [loop]
    for (int iLight = 0; iLight < GetScene().LightCount; iLight++)
    {
        Light light = LoadLight(iLight);
        ProcessLight(light, brdfSurfaceData, lightingTerms);
    }

#endif
}

void DirectLightContribution_Cluster(in Scene scene, in BRDFDataPerSurface brdfSurfaceData, in Surface surface, uint tileAddress, inout Lighting lightingTerms)
{
    
    lightingTerms.Direct.Init(0, 0);
    
#ifdef USE_HARD_CODED_LIGHT
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
    lightingTerms.Direct.Diffuse += (shadow * directLightContribution) * lightColour;
    lightingTerms.Direct.Specular += (shadow * specularLightContribution) * lightColour;
#else
    float zLightFar = 100.0f;
    float4 positionVS = mul(float4(brdfSurfaceData.P, 1.0f), GetCamera().View);
    float linearDepth = (-positionVS.z - GetCamera().GetZNear()) / zLightFar - GetCamera().GetZNear();
    int binIndex = int(linearDepth / BIN_WIDTH);
    uint binValue = GetLightBin(binIndex);
        
    uint minLightId = binValue & 0xFFFF;
    uint maxLightId = (binValue >> 16) & 0xFFFF;
    
    // There is another version that is more optmized
    // From the Master graphics programming book. However, we do liting in pixel shaders so it doesn't work as cleanly.
    // as it makes use of wave instrisics.
    // NOTE(marco): this version has been implemented following:
    // https://www.activision.com/cdn/research/2017_Sig_Improved_Culling_final.pdf
    // See the presentation for more details
    if (minLightId != MAX_NUM_LIGHTS + 1)
    {
        for (uint lightID = minLightId; lightID <= maxLightId; ++lightID)
        {
            uint wordId = lightID / 32;
            uint bitId = lightID % 32;

            if ((GetLightTile(tileAddress + wordId) & (1u << bitId)) != 0)
            {
                Light light = LoadLight(lightID);
                ProcessLight(light, brdfSurfaceData, lightingTerms);
            }
        }
    }
#endif
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

void IndirectLightContribution_Flat(
    float3 ambientTerm,
    float3 specularTerm,
    inout Lighting lightingTerms)
{
    lightingTerms.Indirect.Diffuse = ambientTerm;
    lightingTerms.Indirect.Specular = specularTerm;
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

#endif // __LIGHTING__HLSLI__