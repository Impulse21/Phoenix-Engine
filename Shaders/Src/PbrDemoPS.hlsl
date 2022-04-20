#include "BRDFFunctions.hlsli"

// -- Const buffers ---

#define DRAW_FLAG_ALBEDO        0x01
#define DRAW_FLAG_NORMAL        0x02
#define DRAW_FLAG_ROUGHNESS     0x04
#define DRAW_FLAG_METALLIC      0x08
#define DRAW_FLAG_AO            0x10
#define DRAW_FLAG_TANGENT       0x20
#define DRAW_FLAG_BITANGENT     0x40

struct DrawPushConstant
{
    uint InstanceBufferIndex;
    uint GeometryBufferIndex;
};

struct SceneInfo
{
    matrix ViewProjection;

	// -- 16 byte boundary ----
    
    matrix ShadowViewProjection;

	// -- 16 byte boundary ----
    
    float3 CameraPosition;
    uint _padding;

	// -- 16 byte boundary ----
    
    float3 SunDirection;
    uint _padding1;

	// -- 16 byte boundary ----
    
    float3 SunColour;
    uint IrradianceMapTexIndex;

	// -- 16 byte boundary ----
    
    uint PreFilteredEnvMapTexIndex;
    uint BrdfLUTTexIndex;
    
};

struct Material
{
    float3 AlbedoColour;
    uint AlbedoTexture;

	// -- 16 byte boundary ----
		
    uint MaterialTexture;
    uint RoughnessTexture;
    uint MetalnessTexture;
    uint AOTexture;

	// -- 16 byte boundary ----

    uint NormalTexture;
    float Metalness;
    float Roughness;
    float AO;
    
    // -- 16 byte boundary ----
};

ConstantBuffer<DrawPushConstant> DrawPushConstantCB : register(b0);
ConstantBuffer<SceneInfo> SceneInfoCB : register(b1);
StructuredBuffer<Material> MaterialsSB : register(t2);

Texture2D ShadowMap : register(t10);

Texture2D   Texture2DTable[]    : register(t0, Tex2DSpace);
TextureCube TextureCubeTable[] : register(t0, TexCubeSpace);
ByteAddressBuffer BufferTable[] : register(t0, BufferSpace);

SamplerState SamplerDefault : register(s0);
SamplerState SamplerBrdf : register(s1);
SamplerComparisonState ShadowSampler : register(s2);

struct PSInput
{
    float3 NormalWS : NORMAL;
    float4 Colour : COLOUR;
    float2 TexCoord : TEXCOORD;
    float3 ShadowTexCoord : TEXCOORD1;
    float3 PositionWS : Position;
    float4 TangentWS : TANGENT;
    uint MaterialID : MATERIAL;
};
    
// Constant normal incidence Fresnel factor for all dielectrics.
float GetShadow(float3 shadowMapCoord)
{
    float result = ShadowMap.SampleCmpLevelZero(ShadowSampler, shadowMapCoord.xy, shadowMapCoord.z);
    return result * result;
}

float4 main(PSInput input) : SV_Target
{
    Material material = MaterialsSB[input.MaterialID];
    
    // -- Collect Material Data ---
    float3 albedo = material.AlbedoColour;
    if (material.AlbedoTexture != InvalidDescriptorIndex)
    {
        albedo = Texture2DTable[material.AlbedoTexture].Sample(SamplerDefault, input.TexCoord).xyz;
    }
    
    float metallic = material.Metalness;
    if (material.MetalnessTexture != InvalidDescriptorIndex)
    {
        metallic = Texture2DTable[material.MetalnessTexture].Sample(SamplerDefault, input.TexCoord).r;
    }
    
    float roughness = material.Roughness;
    if (material.RoughnessTexture != InvalidDescriptorIndex)
    {
        roughness = Texture2DTable[material.RoughnessTexture].Sample(SamplerDefault, input.TexCoord).r;
    }
        
    float ao = material.AO;
    if (material.AOTexture != InvalidDescriptorIndex)
    {
        ao = Texture2DTable[material.AOTexture].Sample(SamplerDefault, input.TexCoord).r;
    }
    
    float3 tangent = normalize(input.TangentWS.xyz);
    float3 normal = normalize(input.NormalWS);
    float3 biTangent = cross(normal, tangent.xyz) * input.TangentWS.w;
    if (material.NormalTexture != InvalidDescriptorIndex)
    {
        float3x3 tbn = float3x3(tangent, biTangent, normal);
        normal = Texture2DTable[material.NormalTexture].Sample(SamplerDefault, input.TexCoord).rgb * 2.0 - 1.0;;
        normal = normalize(mul(normal, tbn));
    }
    
    // -- End Material Collection ---
    
    // -- Lighting Model ---
    float3 N = normalize(normal);
    float3 V = normalize(SceneInfoCB.CameraPosition - input.PositionWS);
    float3 R = reflect(-V, N);
    
    // Linear Interpolate the value against the abledo as matallic
    // surfaces reflect their colour.
    float3 F0 = lerp(Fdielectric, albedo, metallic);
    
    // Reflectance equation
    float3 Lo = float3(0.0f, 0.0f, 0.0f);
    {
        // -- Iterate over lights here
        // If this is a point light, calculate vector from light to World Pos
        float3 L = -normalize(SceneInfoCB.SunDirection);
        float3 H = normalize(V + L);
    
        // If point light, calculate attenuation here;
        float3 radiance = SceneInfoCB.SunColour; // * attenuation;
    
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
    if (SceneInfoCB.IrradianceMapTexIndex != InvalidDescriptorIndex &&
        SceneInfoCB.PreFilteredEnvMapTexIndex != InvalidDescriptorIndex &&
        SceneInfoCB.BrdfLUTTexIndex != InvalidDescriptorIndex)
    {
        
        float3 F = FresnelSchlick(saturate(dot(N, V)), F0, roughness);
        // Improvised abmient lighting by using the Env Irradance map.
        float3 irradiance = TextureCubeTable[SceneInfoCB.IrradianceMapTexIndex].Sample(SamplerDefault, N).rgb;
        
        float3 kSpecular = F;
        float3 kDiffuse = 1.0 - kSpecular;
        float3 diffuse = irradiance * albedo;
        
        // Sample both the BRDFLut and Pre-filtered map and combine them together as per the
        // split-sum approximation to get the IBL Specular part.
        float lodLevel = roughness * MaxReflectionLod;
        float3 prefilteredColour =
            TextureCubeTable[SceneInfoCB.PreFilteredEnvMapTexIndex].SampleLevel(SamplerDefault, R, lodLevel).rgb;
        
        float2 brdfTexCoord = float2(saturate(dot(N, V)), roughness);
        
        float2 brdf = Texture2DTable[SceneInfoCB.BrdfLUTTexIndex].Sample(SamplerBrdf, brdfTexCoord).rg;
        
        float3 specular = prefilteredColour * (F * brdf.x + brdf.y);   
        
        ambient = (kDiffuse * diffuse + specular) * ao;
    }
        
    float3 colour = ambient + Lo;
    
    float4 shadowMapCoord = mul(float4(input.PositionWS, 1.0f), SceneInfoCB.ShadowViewProjection);
    // Convert to Texture space
    shadowMapCoord.xyz /= shadowMapCoord.w;
    
    // Apply Bias
    shadowMapCoord.xy = shadowMapCoord.xy * float2(0.5, -0.5) + 0.5;
    
    // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
    // XMMATRIX T(0.5f, 0.0f, 0.0f, 0.0f,
    //            0.0f, -0.5f, 0.0f, 0.0f,
    //            0.0f, 0.0f, 1.0f, 0.0f,
    //            0.5f, 0.5f, 0.0f, 1.0f);
    
    colour = GetShadow(shadowMapCoord.xyz) * colour;
    // colour = GetShadow(input.ShadowTexCoord) * colour;
    
    // Correction for gamma?
    colour = colour / (colour + float3(1.0, 1.0, 1.0));
    colour = pow(colour, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
    
    return float4(colour, 1.0f);
}