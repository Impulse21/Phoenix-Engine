#ifndef __PHX_FULLSCREEN_HELPERS_HLSLI__
#define __PHX_FULLSCREEN_HELPERS_HLSLI__

#if false
inline void CreateFullScreenTriangle(in uint vertexID, out float4 pos, out float2 uv)
{
	// I do now under stand this at the moment?
	// FROM Nvida's Donut Engine
	// uv = float2((vertexID << 1) & 2, vertexID & 2);
	// pos = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);

	// From OpenGL Example -> Uses Triangle strip and 4 vertexIDs
	// uv = float2(vertexID % 2, (vertexID % 4) >> 1);
	// pos = float4((uv.x - 0.5f) * 2, -(uv.y - 0.5f) * 2, 0, 1);

	// From Mini Engine
	// Texture coordinates range [0, 2], but only [0, 1] appears on screen.

	uv = float2(uint2(vertexID, vertexID << 1) & 2);
	pos = float4(lerp(float2(-1, 1), float2(1, -1), uv), 0, 1);
}
#endif

// Creates a full screen triangle from 3 vertices:
inline void CreateFullscreenTriangle_POS(in uint vertexID, out float4 pos)
{
	pos.x = (float)(vertexID / 2) * 4.0 - 1.0;
	pos.y = (float)(vertexID % 2) * 4.0 - 1.0;
	pos.z = 0;
	pos.w = 1;
}


inline void CreateFullscreenTriangle_POS_UV(in uint vertexID, out float4 pos, out float2 uv)
{
	CreateFullscreenTriangle_POS(vertexID, pos);

	uv.x = (float)(vertexID / 2) * 2;
	uv.y = 1 - (float)(vertexID % 2) * 2;
}
#endif // __PHX_FULLSCREEN_HELPERS_HLSLI__