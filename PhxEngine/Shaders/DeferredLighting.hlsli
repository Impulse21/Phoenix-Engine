#ifndef _DEFERRED_LIGHTING_HLSL__
#define _DEFERRED_LIGHTING_HLSL__

#include "Globals.hlsli"
#include "GBuffer.hlsli"
#include "FullScreenHelpers.hlsli"
#include "BRDFFunctions.hlsli"


// TODO: ROOT SIGNATURE

#if USE_RESOURCE_HEAP

#define DeferredLightingRS \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "SRV(t0),"  \
    "DescriptorTable(SRV(t1, numDescriptors = 5)), " \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR),"

#else

#define DeferredLightingRS \
	"RootFlags(0), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "SRV(t0),"  \
    "DescriptorTable(SRV(t1, numDescriptors = 5)), " \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR),"

#endif

Texture2D GBuffer_Depth : register(t1);
Texture2D GBuffer_0     : register(t2);
Texture2D GBuffer_1     : register(t3);
Texture2D GBuffer_2     : register(t4);

// TODO: Remove after testing
Texture2D GBuffer_Debug_Position  : register(t5);

SamplerState DefaultSampler: register(s50);
SamplerState SamplerBrdf : register(s51);

struct PSInput
{
	float2 UV : TEXCOORD;
	float4 Position : SV_POSITION;
};

#ifdef DEFERRED_LIGHTING_COMPILE_VS
[RootSignature(DeferredLightingRS)]
PSInput main(uint id : SV_VertexID)
{
    PSInput output;

    CreateFullScreenTriangle(id, output.Position, output.UV);

    return output;
}

#endif

#ifdef DEFERRED_LIGHTING_COMPILE_PS

inline float3 CalculateDirectAnalyticalContribution(float3 N, float3 V, float3 R, float3 F0, float3 L, float3 Radiance, SurfaceProperties surfaceProperties)
{
    float3 H = normalize(V + L);

    // Calculate surfaceProperties.Normal Distribution Term
    float NDF = DistributionGGX(N, H, surfaceProperties.Roughness);

    // Calculate Geometry Term
    float G = GeometrySmith(N, V, L, surfaceProperties.Roughness);

    // Calculate Fersnel Term
    float3 F = FresnelSchlick(saturate(dot(H, V)), F0);

    // Now calculate Cook-Torrance BRDF
    float3 numerator = NDF * G * F;

    // NOTE: we add 0.0001 to the denomiator to prevent a divide by zero in the case any dot product ends up zero
    float denominator = 4.0 * saturate(dot(N, V)) * saturate(dot(N, L)) + 0.0001;

    float3 specular = numerator / denominator;

    // Now we can calculate the light's constribution to the reflectance equation. Since Fersnel Value directly corresponds to
    // Ks, we ca use F to denote the specular contribution of any light that hits the surface.
    // we can now deduce what the diffuse contribution is as 1.0 = KSpecular + kDiffuse;
    float3 kSpecular = F;
    float3 KDiffuse = float3(1.0, 1.0, 1.0) - kSpecular;
    KDiffuse *= 1.0f - surfaceProperties.Metalness;

    float NdotL = saturate(dot(N, L));
    return (KDiffuse * (surfaceProperties.Albedo / PI) + specular) * Radiance * NdotL;
}

[RootSignature(DeferredLightingRS)]
float4 main(PSInput input) : SV_TARGET
{
    Camera camera = GetCamera();
    SceneData scene = GetScene();

    float4 gbufferChannels[NUM_GBUFFER_CHANNELS];
    gbufferChannels[0] = GBuffer_0.Sample(SamplerDefault, input.UV);
    gbufferChannels[1] = GBuffer_1.Sample(SamplerDefault, input.UV);
    gbufferChannels[2] = GBuffer_2.Sample(SamplerDefault, input.UV);

    // TODO: Remove after testing
    float3 debugWorldPosition = GBuffer_Debug_Position.Sample(SamplerDefault, input.UV).xyz;

    SurfaceProperties surfaceProperties = DecodeGBuffer(gbufferChannels);
    float3 surfacePosition = ReconstructWorldPosition(camera, input.UV, GBuffer_Depth[input.UV].x);
    surfacePosition = debugWorldPosition;

    // -- Lighting Model ---
    float3 N = normalize(surfaceProperties.Normal);
    float3 V = normalize((float3)GetCamera().CameraPosition - surfacePosition);
    float3 R = reflect(-V, N);

    // Linear Interpolate the value against the abledo as matallic
    // surfaces reflect their colour.
    float3 F0 = lerp(Fdielectric, surfaceProperties.Albedo, surfaceProperties.Metalness);

    // Reflectance equation
    float3 Lo = float3(0.0f, 0.0f, 0.0f);
    {
#ifdef USE_HARD_CODED_LIGHT
        // If point light, calculate attenuation here;
        float3 radiance = float3(1.0f, 1.0f, 1.0f); // * attenuation;

        // If this is a point light, calculate vector from light to World Pos
        float3 L = -normalize(float3(0.0f, -1.0f, 0.0f));
        float3 H = normalize(V + L);

        // Calculate surfaceProperties.Normal Distribution Term
        float NDF = DistributionGGX(N, H, surfaceProperties.Roughness);

        // Calculate Geometry Term
        float G = GeometrySmith(N, V, L, surfaceProperties.Roughness);

        // Calculate Fersnel Term
        float3 F = FresnelSchlick(saturate(dot(H, V)), F0);

        // Now calculate Cook-Torrance BRDF
        float3 numerator = NDF * G * F;

        // NOTE: we add 0.0001 to the denomiator to prevent a divide by zero in the case any dot product ends up zero
        float denominator = 4.0 * saturate(dot(N, V)) * saturate(dot(N, L)) + 0.0001;

        float3 specular = numerator / denominator;

        // Now we can calculate the light's constribution to the reflectance equation. Since Fersnel Value directly corresponds to
        // Ks, we ca use F to denote the specular contribution of any light that hits the surface.
        // we can now deduce what the diffuse contribution is as 1.0 = KSpecular + kDiffuse;
        float3 kSpecular = F;
        float3 KDiffuse = float3(1.0, 1.0, 1.0) - kSpecular;
        KDiffuse *= 1.0f - surfaceProperties.Metalness;

        float NdotL = saturate(dot(N, L));
        Lo += (KDiffuse * (surfaceProperties.Albedo / PI) + specular) * radiance * NdotL;
        // -- Direct Lights ---
#else
        [loop]
        for (int nLights = 0; nLights < scene.NumLights; nLights++)
        {
            ShaderLight light = LoadLight(nLights);

            switch (light.GetType())
            {
            case ENTITY_TYPE_DIRECTIONALLIGHT:
            {
                float3 L = -normalize(light.GetDirection());
                float3 radiance = light.GetColour().rgb * light.GetEnergy(); // *shadow;

                Lo += CalculateDirectAnalyticalContribution(N, V, R, F0, L, radiance, surfaceProperties);
            }
                break;

            case ENTITY_TYPE_OMNILIGHT:
            {
                float3 L = normalize(light.Position - surfacePosition);

                // TODO: Ignore any lights that Range is less then distance.
                const float dist2 = dot(L, L);
                const float range2 = light.GetRange() * light.GetRange();

                // Calculate attenutaion
                const float att = saturate(1 - (dist2 / range2));
                const float attenuation = att * att;

                // Calculate attenutaion
                float3 radiance = light.GetColour().rgb * light.GetEnergy(); // *shadow;
                radiance *= attenuation;

                Lo += CalculateDirectAnalyticalContribution(N, V, R, F0, L, radiance, surfaceProperties);
            }
                break;

            case ENTITY_TYPE_SPOTLIGHT:
            {
                float3 L = normalize(light.Position - surfacePosition);
                const float dist2 = dot(L, L);
                const float range2 = light.GetRange() * light.GetRange();

                // TODO: Ignore any lights that Range is less then distance.
                const float spotFactor = dot(L, light.GetDirection());
                const float spotCutoff = light.GetConeAngleCos();

                // Calculate attenutaion
                const float att = saturate(1 - (dist2 / range2));
                float attenuation = att * att;
                attenuation *= saturate((1 - (1 - spotFactor) * 1 / (1 - spotCutoff)));

                float3 radiance = light.GetColour().rgb * light.GetEnergy(); // *shadow;
                radiance *= attenuation;

                Lo += CalculateDirectAnalyticalContribution(N, V, R, F0, L, radiance, surfaceProperties);
            }
                break;
            }
        }
#endif
    }

    // Improvised abmient lighting by using the Env Irradance map.
    float3 ambient = float(0.01).xxx * surfaceProperties.Albedo * surfaceProperties.AO;
    if (GetScene().IrradianceMapTexIndex != InvalidDescriptorIndex &&
        GetScene().PreFilteredEnvMapTexIndex != InvalidDescriptorIndex &&
        FrameCB.BrdfLUTTexIndex != InvalidDescriptorIndex)
    {

        float3 F = FresnelSchlick(saturate(dot(N, V)), F0, surfaceProperties.Roughness);
        // Improvised abmient lighting by using the Env Irradance map.
        float3 irradiance = ResourceHeap_GetTextureCube(GetScene().IrradianceMapTexIndex).Sample(SamplerDefault, N).rgb;

        float3 kSpecular = F;
        float3 kDiffuse = 1.0 - kSpecular;
        float3 diffuse = irradiance * surfaceProperties.Albedo;

        // Sample both the BRDFLut and Pre-filtered map and combine them together as per the
        // split-sum approximation to get the IBL Specular part.
        float lodLevel = surfaceProperties.Roughness * MaxReflectionLod;
        float3 prefilteredColour =
            ResourceHeap_GetTextureCube(GetScene().PreFilteredEnvMapTexIndex).SampleLevel(SamplerDefault, R, lodLevel).rgb;

        float2 brdfTexCoord = float2(saturate(dot(N, V)), surfaceProperties.Roughness);

        float2 brdf = ResourceHeap_GetTexture2D(FrameCB.BrdfLUTTexIndex).Sample(SamplerBrdf, brdfTexCoord).rg;

        float3 specular = prefilteredColour * (F * brdf.x + brdf.y);

        ambient = (kDiffuse * diffuse + specular) * surfaceProperties.AO;
    }

    float3 colour = ambient + Lo;

    // float4 shadowMapCoord = mul(float4(input.PositionWS, 1.0f), GetCamera().ShadowViewProjection);
    // Convert to Texture space
    // shadowMapCoord.xyz /= shadowMapCoord.w;

    // Apply Bias
    // shadowMapCoord.xy = shadowMapCoord.xy * float2(0.5, -0.5) + 0.5;

    // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
    // XMMATRIX T(0.5f, 0.0f, 0.0f, 0.0f,
    //            0.0f, -0.5f, 0.0f, 0.0f,
    //            0.0f, 0.0f, 1.0f, 0.0f,
    //            0.5f, 0.5f, 0.0f, 1.0f);

    // colour = GetShadow(shadowMapCoord.xyz) * colour;
    // colour = GetShadow(input.ShadowTexCoord) * colour;

    // Correction for gamma?
    colour = colour / (colour + float3(1.0, 1.0, 1.0));
    colour = pow(colour, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

    return float4(colour, 1.0f);
}
#endif

#ifdef DEFERRED_LIGHTING_COMPILE_CS

#endif

#endif // _DEFERRED_LIGHTING_HLSL__