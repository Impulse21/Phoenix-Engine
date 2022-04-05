#ifndef __PHX_SHADER_INTEROP_STRUCTURES_HLSLI__
#define __PHX_SHADER_INTEROP_STRUCTURES_HLSLI__

#include "ShaderInterop.h"

#ifdef __cplusplus
namespace Shader
{
#endif

// -- Enums ---
static const uint ENTITY_TYPE_DIRECTIONALLIGHT = 0;
static const uint ENTITY_TYPE_OMNILIGHT = 1;
static const uint ENTITY_TYPE_SPOTLIGHT = 2;

struct ShaderTransform
{
	float4 Mat0;
	float4 Mat1;
	float4 Mat2;

	void init()
	{
		Mat0 = float4(1, 0, 0, 0);
		Mat1 = float4(0, 1, 0, 0);
		Mat2 = float4(0, 0, 1, 0);
	}
	void Create(float4x4 mat)
	{
		Mat0 = float4(mat._11, mat._21, mat._31, mat._41);
		Mat1 = float4(mat._12, mat._22, mat._32, mat._42);
		Mat2 = float4(mat._13, mat._23, mat._33, mat._43);
	}
	float4x4 GetMatrix()
#ifdef __cplusplus
		const
#endif // __cplusplus
	{
		return float4x4(
			Mat0.x, Mat0.y, Mat0.z, Mat0.w,
			Mat1.x, Mat1.y, Mat1.z, Mat1.w,
			Mat2.x, Mat2.y, Mat2.z, Mat2.w,
			0, 0, 0, 1
		);
	}
};

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
	uint MaterialIndex;
	uint NumIndices;
	uint NumVertices;
	uint IndexOffset;

	// -- 16 byte boundary ---
	uint VertexBufferIndex;
	uint PositionOffset;
	uint TexCoordOffset;
	uint NormalOffset;

	// -- 16 byte boundary ---
	uint TangentOffset;
	uint3 _padding;

	// -- 16 byte boundary ---
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


struct GeometryPassPushConstants
{
	float4x4 WorldTransform;

	uint GeometryIndex;
	uint MeshIndex;
};

#ifdef __cplusplus
}
#endif
#endif // __PHX_SHADER_INTEROP_STRUCTURES_HLSLI__