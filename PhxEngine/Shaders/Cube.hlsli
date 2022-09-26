#ifndef __CUBE_HLSL__
#define __CUBE_HLSL__
static const float4 sCube[] =
{
	float4(-1,-1,1, 1), float4(1,-1,1, 1), float4(1,1,1, 1),
	float4(-1,-1,1, 1), float4(1,1,1, 1), float4(-1,1,1, 1),
	float4(1,-1,1, 1), float4(1,-1,-1, 1), float4(1,1,-1, 1),
	float4(1,-1,1, 1), float4(1,1,-1, 1), float4(1,1,1, 1),
	float4(1,-1,-1, 1), float4(-1,-1,-1, 1), float4(-1,1,-1, 1),
	float4(1,-1,-1, 1), float4(-1,1,-1, 1), float4(1,1,-1, 1),
	float4(-1,-1,-1, 1), float4(-1,-1,1, 1), float4(-1,1,1, 1),
	float4(-1,-1,-1, 1), float4(-1,1,1, 1), float4(-1,1,-1, 1),
	float4(-1,1,1, 1), float4(1,1,1, 1), float4(1,1,-1, 1),
	float4(-1,1,1, 1), float4(1,1,-1, 1), float4(-1,1,-1, 1),
	float4(1,-1,1, 1), float4(-1,-1,-1, 1), float4(1,-1,-1, 1),
	float4(1,-1,1, 1), float4(-1,-1, 1, 1), float4(-1,-1,-1, 1),
};

#endif