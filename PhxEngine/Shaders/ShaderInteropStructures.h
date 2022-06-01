#ifndef __PHX_SHADER_INTEROP_STRUCTURES_HLSLI__
#define __PHX_SHADER_INTEROP_STRUCTURES_HLSLI__

#include "ShaderInterop.h"

#ifdef __cplusplus

#include <Directxpackedvector.h>

namespace Shader
{
#endif

// -- Enums ---
static const uint ENTITY_TYPE_DIRECTIONALLIGHT = 0;
static const uint ENTITY_TYPE_OMNILIGHT = 1;
static const uint ENTITY_TYPE_SPOTLIGHT = 2;

struct ShaderLight
{
	float3 Position;
	uint Type8_Flags8_Range16; // <Range_16><flags_8><type_8>

	// -- 16 byte boundary ----

	uint Energy16_X16; // <free_16><range_8>
	uint ColorPacked;
	uint2 _Padding;

#ifndef __cplusplus
	inline float4 GetColour()
	{
		float4 refVal;

		refVal.x = (float)((ColorPacked >> 0) & 0xFF) / 255.0f;
		refVal.y = (float)((ColorPacked >> 8) & 0xFF) / 255.0f;
		refVal.z = (float)((ColorPacked >> 16) & 0xFF) / 255.0f;
		refVal.w = (float)((ColorPacked >> 24) & 0xFF) / 255.0f;

		return refVal;
	}

	inline uint GetType()
	{
		return Type8_Flags8_Range16 & 0xFF;
	}

	inline uint GetFlags()
	{
		return (Type8_Flags8_Range16 >> 8) & 0xFF;
	}

	inline float GetRange()
	{
		return f16tof32((Type8_Flags8_Range16 >> 16) & 0xFFFF);
	}

	inline float GetEnergy()
	{
		return f16tof32(Energy16_X16 & 0xFFFF);
	}

#else
	inline void SetType(uint type)
	{
		Type8_Flags8_Range16 |= type & 0xFF;
	}

	inline void SetFlags(uint flags)
	{
		Type8_Flags8_Range16 |= (flags & 0xFF) << 8;
	}

	inline void SetRange(float value)
	{
		Type8_Flags8_Range16 |= DirectX::PackedVector::XMConvertFloatToHalf(value) << 16;
	}

	inline void SetEnergy(float value)
	{
		Energy16_X16 |= DirectX::PackedVector::XMConvertFloatToHalf(value);
	}
#endif
};

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

struct FontVertex
{
	float2 Position;
	float2 TexCoord;
};

struct FontDrawInfo
{
	float4x4 Mvp;

	// -- 16 byte boundary ----
	uint ColorPacked;
	uint TextureIndex;

#ifndef __cplusplus
	inline float4 GetColour()
	{
		float4 refVal;

		refVal.x = (float)((ColorPacked >> 0) & 0xFF) / 255.0f;
		refVal.y = (float)((ColorPacked >> 8) & 0xFF) / 255.0f;
		refVal.z = (float)((ColorPacked >> 16) & 0xFF) / 255.0f;
		refVal.w = (float)((ColorPacked >> 24) & 0xFF) / 255.0f;

		return refVal;
	}
#endif
};

struct SceneData
{
	uint MeshBufferIndex;
	uint GeometryBufferIndex;
	uint MaterialBufferIndex;
	uint IrradianceMapTexIndex;

	// -- 16 byte boundary ----

	uint PreFilteredEnvMapTexIndex;
	uint NumLights;
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
	uint EmissiveColourPacked;
	uint2 _Padding;

#ifndef __cplusplus
	inline float4 GetEmissiveColour()
	{
		float4 refVal;

		refVal.x = (float)((EmissiveColourPacked >> 0) & 0xFF) / 255.0f;
		refVal.y = (float)((EmissiveColourPacked >> 8) & 0xFF) / 255.0f;
		refVal.z = (float)((EmissiveColourPacked >> 16) & 0xFF) / 255.0f;
		refVal.w = (float)((EmissiveColourPacked >> 24) & 0xFF) / 255.0f;

		return refVal;
	}
#endif
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

#define DRAW_FLAG_ALBEDO        0x001
#define DRAW_FLAG_NORMAL        0x002
#define DRAW_FLAG_ROUGHNESS     0x004
#define DRAW_FLAG_METALLIC      0x008
#define DRAW_FLAG_AO            0x010
#define DRAW_FLAG_TANGENT       0x020
#define DRAW_FLAG_BITANGENT     0x040
#define DRAW_FLAG_EMISSIVE		0x080
#define DRAW_FLAGS_DISABLE_IBL  0x100

struct GeometryPassPushConstants
{
	float4x4 WorldTransform;

	uint GeometryIndex;
	uint MeshIndex;
	uint DrawFlags;
};

struct ImagePassPushConstants
{

};
#ifdef __cplusplus
}
#endif
#endif // __PHX_SHADER_INTEROP_STRUCTURES_HLSLI__