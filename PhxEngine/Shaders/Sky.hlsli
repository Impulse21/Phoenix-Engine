#ifndef __SKY_HLSL__
#define __SKY_HLSL__

#include "Globals.hlsli"
#include "Defines.hlsli"


#define PHX_ENGINE_SKY_CAPTURE_ROOTSIGNATURE \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
	"CBV(b0), " \
	"CBV(b1), " \
    "CBV(b2),"  \
	"StaticSampler(s50, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s51, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR),"


ConstantBuffer<RenderCams> CubemapRenderCamsCB : register(b2);


/*

	Non physical based atmospheric scattering made by robobo1221
	Site: http://www.robobo1221.net/shaders
	Shadertoy: http://www.shadertoy.com/user/robobo1221

	Credit: https://wickedengine.net/
*/
float3 GetAtmosphericScattering(float3 V, float3 sunDirection, float3 sunColour, bool enableDrawSun, bool darkEnabled) 
{
	const float3 skyColour = GetZenithColour();
	const bool sunPresent = any(sunColour);
	const bool drawSun = sunPresent && enableDrawSun;

	const float multiScatterPhase = 0.1;
	const float atmosphereDensity = 0.7;

	const float zenith = V.y;
	const float sunScatter = saturate(sunDirection.y + 0.1f); //Higher numbers result in more anisotropic scattering

	const float zenithDensity = atmosphereDensity / pow(max(0.000001f, zenith), 0.75f);
	const float sunScatterDensity = atmosphereDensity / pow(max(0.000001f, sunScatter), 0.75f);

	const float3 aberration = float3(0.39, 0.57, 1.0); // the chromatic aberration effect on the horizon-zenith fade line
	const float3 skyAbsorption = saturate(exp2(aberration * -zenithDensity) * 2.0f); // gradient on horizon
	const float3 sunAbsorption = sunPresent ? saturate(skyColour * exp2(aberration * -sunScatterDensity) * 2.0f) : 1; // gradient of sun when it's getting below horizon
	const float sunAmount = distance(V, sunDirection); // sun falloff descreasing from mid point
	const float rayleigh = sunPresent ? 1.0 + pow(1.0 - saturate(sunAmount), 2.0) * PI * 0.5 : 1;
	const float mieDisk = saturate(1.0 - pow(sunAmount, 0.1));
	const float3 mie = mieDisk * mieDisk * (3.0 - 2.0 * mieDisk) * 2.0 * PI * sunAbsorption;

	float3 totalColour = lerp(GetHorizonColour(), GetZenithColour() * zenithDensity * rayleigh, skyAbsorption);
	totalColour = lerp(totalColour * skyAbsorption, totalColour, sunScatter); // when sun goes below horizon, absorb sky color
	if (drawSun)
	{
		const float3 sun = smoothstep(0.03, 0.026, sunAmount) * skyColour * 50.0 * skyAbsorption; // sun disc
		totalColour += sun;
		totalColour += mie;
	}

	totalColour *= (sunAbsorption + length(sunAbsorption)) * 0.5f; // when sun goes below horizon, fade out whole sky
	totalColour *= 0.25; // exposure level


	if (darkEnabled)
	{
		totalColour = max(pow(saturate(dot(sunDirection, V)), 64) * sunColour, 0) * skyAbsorption;
	}

	return totalColour;
}

inline float3 GetProceduralSkyColour(in float3 viewDir, bool sunEnabled = true, bool darkEnabled = false)
{
	if (GetFrame().Option & FRAME_OPTION_BIT_SIMPLE_SKY)
	{
		return lerp(GetHorizonColour(), GetZenithColour(), saturate(viewDir.y * 0.5f + 0.5f));
	}

	const float3 sunDirection = GetAtmosphere().SunDirection;
	const float3 sunColour = GetAtmosphere().SunColour;
    return GetAtmosphericScattering(viewDir, sunDirection, sunColour, sunEnabled, darkEnabled);
}

#endif // __SKY_HLSL__