#ifndef __PHX_SHADER_INTEROP_STRUCTURES_HLSLI__
#define __PHX_SHADER_INTEROP_STRUCTURES_HLSLI__

#include "ShaderInterop.h"

#ifdef __cplusplus
namespace Shader
{
#endif

struct ImguiDrawInfo
{
	float4x4 Mvp;
	uint TextureIndex;
};

struct SceneData
{
	uint MeshBufferIndex;
	uint GeometryBufferIndex;
	uint MaterialBufferIndex;
	uint IrradianceMapTexIndex;

	// -- 16 byte boundary ----

	uint PreFilteredEnvMapTexIndex;
	uint _padding0;
	uint _padding1;
	uint _padding2;

	// -- 16 byte boundary ----
};

struct MaterialData
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

	uint Flags;
};

struct Mesh
{
	uint VbPositionBufferIndex;
	uint VbTexCoordBufferIndex;
	uint VbNormalBufferIndex;
	uint VbTangentBufferIndex;

	// -- 16 byte boundary ----
	uint Flags;

	uint _padding0;
	uint _padding1;
	uint _padding2;

	// -- 16 byte boundary ----
};

struct Geometry
{
	uint VertexOffset;
	uint MaterialIndex;
};

struct Frame
{
	uint BrdfLUTTexIndex;
	uint _padding0;
	uint _padding1;
	uint _padding2;

	// -- 16 byte boundary ----
	
	SceneData Scene;
};

struct Camera
{
	float4x4 ViewProjection;

	// -- 16 byte boundary ----

	float4x4 ShadowViewProjection;

	// -- 16 byte boundary ----

	float3 CameraPosition;
};


struct MeshRenderPushConstant
{
	uint GeometryIndex;
	uint MeshIndex;
	float4x4 WorldTransform;
};

#ifdef __cplusplus
}
#endif
#endif // __PHX_SHADER_INTEROP_STRUCTURES_HLSLI__