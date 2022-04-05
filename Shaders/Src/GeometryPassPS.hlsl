
#include "Globals.hlsli"
#include "BRDFFunctions.hlsli"


// -- Const buffers ---

struct PSInput
{
    float3 NormalWS : NORMAL;
    float4 Colour : COLOUR;
    float2 TexCoord : TEXCOORD;
    float3 PositionWS : Position;
    float4 TangentWS : TANGENT;
    uint MaterialID : MATERIAL;
};
    
// Constant normal incidence Fresnel factor for all dielectrics.
static const float Fdielectric = 0.04f;
static const float MaxReflectionLod = 7.0f;

float4 main(PSInput input) : SV_Target
{
    MaterialData material = LoadMaterial(input.MaterialID);
    
    // -- Collect Material Data ---
    float3 albedo = material.AlbedoColour;
    if (material.AlbedoTexture != InvalidDescriptorIndex)
    {
        albedo = ResourceHeap_Texture2D[material.AlbedoTexture].Sample(SamplerDefault, input.TexCoord).xyz;
    }

    float metallic = material.Metalness;
    if (material.MetalnessTexture != InvalidDescriptorIndex)
    {
        metallic = ResourceHeap_Texture2D[material.MetalnessTexture].Sample(SamplerDefault, input.TexCoord).r;
    }
    
    float roughness = material.Roughness;
    if (material.RoughnessTexture != InvalidDescriptorIndex)
    {
        roughness = ResourceHeap_Texture2D[material.RoughnessTexture].Sample(SamplerDefault, input.TexCoord).r;
    }
        
    float ao = material.AO;
    if (material.AOTexture != InvalidDescriptorIndex)
    {
        ao = ResourceHeap_Texture2D[material.AOTexture].Sample(SamplerDefault, input.TexCoord).r;
    }
    
    float3 tangent = normalize(input.TangentWS.xyz);
    float3 normal = normalize(input.NormalWS);
    float3 biTangent = cross(normal, tangent.xyz) * input.TangentWS.w;
    if (material.NormalTexture != InvalidDescriptorIndex)
    {
        float3x3 tbn = float3x3(tangent, biTangent, normal);
        normal = ResourceHeap_Texture2D[material.NormalTexture].Sample(SamplerDefault, input.TexCoord).rgb * 2.0 - 1.0;;
        normal = normalize(mul(normal, tbn));
    }


    // -- End Material Collection ---
    
    // -- Lighting Model ---
    float3 N = normalize(normal);
    float3 V = normalize(GetCamera().CameraPosition - input.PositionWS);
    float3 R = reflect(-V, N);
    
    // Linear Interpolate the value against the abledo as matallic
    // surfaces reflect their colour.
    float3 F0 = lerp(Fdielectric, albedo, metallic);
    
    float3 sunDir = float3(0.0f, -1.0f, 1.0f);
    // Reflectance equation
    float3 Lo = float3(0.0f, 0.0f, 0.0f);
    {
        // -- Iterate over lights here
        // If this is a point light, calculate vector from light to World Pos
        float3 L = -normalize(sunDir);
        float3 H = normalize(V + L);
    
        // If point light, calculate attenuation here;
        float3 radiance = float3(1.0f, 1.0f, 1.0f);// * attenuation;
    
        // Calculate Normal Distribution Term
        float NDF = DistributionGGX(N, H, roughness);
    
        // Calculate Geometry Term
        float G = GeometrySmith(N, V, L, roughness);
    
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
        KDiffuse *= 1.0f - metallic;
    
        float NdotL = saturate(dot(N, L));
        Lo += (KDiffuse * (albedo / PI) + specular) * radiance * NdotL;
    }
    // -- End light iteration
    
    
    // Improvised abmient lighting by using the Env Irradance map.
    float3 ambient = float(0.03).xxx * albedo * ao;
    if (GetScene().IrradianceMapTexIndex != InvalidDescriptorIndex &&
        GetScene().PreFilteredEnvMapTexIndex != InvalidDescriptorIndex &&
        GetFrame().BrdfLUTTexIndex != InvalidDescriptorIndex)
    {
        
        float3 F = FresnelSchlick(saturate(dot(N, V)), F0, roughness);
        // Improvised abmient lighting by using the Env Irradance map.
        float3 irradiance = ResourceHeap_TextureCube[GetScene().IrradianceMapTexIndex].Sample(SamplerDefault, N).rgb;
        
        float3 kSpecular = F;
        float3 kDiffuse = 1.0 - kSpecular;
        float3 diffuse = irradiance * albedo;
        
        // Sample both the BRDFLut and Pre-filtered map and combine them together as per the
        // split-sum approximation to get the IBL Specular part.
        float lodLevel = roughness * MaxReflectionLod;
        float3 prefilteredColour =
            ResourceHeap_TextureCube[GetScene().PreFilteredEnvMapTexIndex].SampleLevel(SamplerDefault, R, lodLevel).rgb;
        
        float2 brdfTexCoord = float2(saturate(dot(N, V)), roughness);
        
        float2 brdf = ResourceHeap_Texture2D[GetFrame().BrdfLUTTexIndex].Sample(SamplerBrdf, brdfTexCoord).rg;
        
        float3 specular = prefilteredColour * (F * brdf.x + brdf.y);   
        
        ambient = (kDiffuse * diffuse + specular) * ao;
    }
        
    float3 colour = ambient + Lo;
    
    // float4 shadowMapCoord = mul(float4(input.PositionWS, 1.0f), SceneInfoCB.ShadowViewProjection);
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