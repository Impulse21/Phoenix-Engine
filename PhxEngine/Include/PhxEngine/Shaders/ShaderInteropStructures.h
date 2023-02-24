#ifndef __PHX_SHADER_INTEROP_STRUCTURES_HLSLI__
#define __PHX_SHADER_INTEROP_STRUCTURES_HLSLI__

#ifdef __cplusplus

#include "ShaderInterop.h"

#else 

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#endif 

#ifdef __cplusplus

#include <Directxpackedvector.h>

namespace Shader
{
#endif

	// -- Enums ---
	static const uint ENTITY_TYPE_DIRECTIONALLIGHT = 0;
	static const uint ENTITY_TYPE_OMNILIGHT = 1;
	static const uint ENTITY_TYPE_SPOTLIGHT = 2;

	static const uint SHADER_LIGHT_ENTITY_COUNT = 256;

	static const uint MATRIX_COUNT = 128;

	// This is restricted to 256 as the Visbuffer is only able to store upt to 256 PrimIDs
	static const uint MESHLET_TRIANGLE_COUNT = 256u; 

	// -- Groups ---
	static const uint GENERATE_MIP_CHAIN_2D_BLOCK_SIZE = 8;

	// -- Frame Options ---
	static const uint FRAME_OPTION_BIT_SIMPLE_SKY = 1 << 0;
	static const uint FRAME_OPTION_BIT_IBL = 1 << 1;

	struct Atmosphere
	{
		float3 SunColour;
		uint _padding;
		// -- 16 byte boundary ----

		float3 SunDirection;
		uint _padding1;
		// -- 16 byte boundary ----

		float3 HorizonColour;
		uint _padding2;
		// -- 16 byte boundary ----

		float3 ZenithColour;
		uint _padding3;
		// -- 16 byte boundary ----

		float3 AmbientColour;
		uint _padding4;
		// -- 16 byte boundary ----
	};

	struct Scene
	{
		uint MeshInstanceBufferIndex;
		uint GeometryBufferIndex;
		uint MaterialBufferIndex;
		uint MeshletBufferIndex;

		// -- 16 byte boundary ----

		uint PreFilteredEnvMapTexIndex;
		uint EnvMapArray;
		uint EnvMap_NumMips;
		uint RT_TlasIndex;

		// --- 16 byte boundary ---
		uint IrradianceMapTexIndex;
		uint _padding;
		uint __pading;
		uint ___padding;

		// -- 16 byte boundary ----
		Atmosphere AtmosphereData;
	};


	struct Scene_NEW
	{
		uint ObjectBufferIdx;
		uint GeometryBufferIdx;
		uint MaterialBufferIdx;
		uint MeshletBufferIndex;

		// -- 16 byte boundary ----
		uint GlobalVertexBufferIdx;
		uint GlobalIndexxBufferIdx;
		uint DrawPacketBufferIdx;
	};

	// -- Common Structurs ---
	struct Frame_NEW
	{
		// -- 16 byte boundary ----
		Scene_NEW SceneData;
	};

	struct Frame
	{
		uint Option;
		uint BrdfLUTTexIndex;
		uint LightEntityDescritporIndex;
		uint LightDataOffset;

		// -- 16 byte boundary ----
		uint LightCount;
		uint MatricesDescritporIndex;
		uint MatricesDataOffset;
		uint _padding;

		// -- 16 byte boundary ----
		Scene SceneData;
	};

	struct Camera
	{
		float4x4 ViewProjection;

		// -- 16 byte boundary ----
		float4x4 ViewProjectionInv;

		// -- 16 byte boundary ----
		float4x4 ProjInv;

		// -- 16 byte boundary ----
		float4x4 ViewInv;

		// -- 16 byte boundary ----

		float4x4 ShadowViewProjection;

		// -- 16 byte boundary ----

#ifndef __cplusplus
		inline float3 GetPosition()
		{
			return ViewInv[3].xyz;
		}
#endif 
	};

	// -- Common Buffers END --- 
	struct ShaderLight
	{
		float3 Position;
		uint Type8_Flags8_Range16; // <Range_16><flags_8><type_8>

		// -- 16 byte boundary ----
		uint2 Direction16_ConeAngleCos16;
		uint Intensity16_X16; // <free_16><range_8>
		uint ColorPacked;
		uint Indices16_Cascades16; // Not current packed

		// -- 16 byte boundary ----
		float4 ShadowAtlasMulAdd;

		// -- 16 byte boundary ----
		// Near first 16 bits, far being the lather
		uint CubemapDepthRemapPacked;
		float Intensity;
		float Range;
		float ConeAngleCos;
		// -- 16 byte boundary ----

		float AngleScale;
		float AngleOffset;
		uint CascadeTextureIndex;

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

		inline float3 GetDirection()
		{
			return float3(
				f16tof32(Direction16_ConeAngleCos16.x & 0xFFFF),
				f16tof32((Direction16_ConeAngleCos16.x >> 16) & 0xFFFF),
				f16tof32(Direction16_ConeAngleCos16.y & 0xFFFF)
			);
		}

		inline float GetConeAngleCos()
		{
			// return f16tof32((Direction16_ConeAngleCos16.y >> 16) & 0xFFFF);
			return ConeAngleCos;
		}

		inline uint GetType()
		{
			return Type8_Flags8_Range16 & 0xFF;
		}

		inline uint GetFlags()
		{
			return (Type8_Flags8_Range16 >> 8) & 0xFF;
		}

		inline uint GetIndices()
		{
			return Indices16_Cascades16 & 0xFFFF;
		}

		inline uint GetNumCascades()
		{
			return (Indices16_Cascades16 >> 16u) & 0xFFFF;
		}

		inline float GetRange()
		{
			// return f16tof32((Type8_Flags8_Range16 >> 16) & 0xFFFF);
			return Range;
		}

		inline float GetIntensity()
		{
			// return f16tof32(Energy16_X16 & 0xFFFF);
			return Intensity;
		}

		inline float GetAngleScale()
		{
			// return f16tof32(remap);
			return AngleScale;
		}

		inline float GetAngleOffset()
		{
			return AngleOffset;
		}

		inline float GetCubemapDepthRemapNear(float value)
		{
			return f16tof32((CubemapDepthRemapPacked) & 0xFFFF);
		}

		inline float GetCubemapDepthRemapFar(float value)
		{
			return f16tof32((CubemapDepthRemapPacked >> 16u) & 0xFFFF);
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
			// Type8_Flags8_Range16 |= DirectX::PackedVector::XMConvertFloatToHalf(value) << 16;
			Range = value;
		}

		inline void SetIntensity(float value)
		{
			// Intensity16_X16 |= DirectX::PackedVector::XMConvertFloatToHalf(value);
			Intensity = value;
		}

		inline void SetDirection(float3 value)
		{
			Direction16_ConeAngleCos16.x |= DirectX::PackedVector::XMConvertFloatToHalf(value.x);
			Direction16_ConeAngleCos16.x |= DirectX::PackedVector::XMConvertFloatToHalf(value.y) << 16u;
			Direction16_ConeAngleCos16.y |= DirectX::PackedVector::XMConvertFloatToHalf(value.z);
		}

		inline void SetConeAngleCos(float value)
		{
			Direction16_ConeAngleCos16.y |= DirectX::PackedVector::XMConvertFloatToHalf(value) << 16u;
			ConeAngleCos = value;
		}

		inline void SetAngleScale(float value)
		{
			AngleScale = value;
		}

		inline void SetAngleOffset(float value)
		{
			AngleOffset = value;
		}

		inline void SetCubemapDepthRemapNear(float value)
		{
			CubemapDepthRemapPacked |= DirectX::PackedVector::XMConvertFloatToHalf(value);
		}

		inline void SetCubemapDepthRemapFar(float value)
		{
			CubemapDepthRemapPacked |= DirectX::PackedVector::XMConvertFloatToHalf(value) << 16u;
		}

		inline void SetIndices(uint value)
		{
			this->Indices16_Cascades16 |= (value & 0xFFFF);
		}

		inline void SetNumCascades(uint value)
		{
			this->Indices16_Cascades16 |= (value & 0xFFFF) << 16u;
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

	struct MeshInstance
	{
		float4x4 WorldMatrix;
		// -- 16 byte boundary ----

		uint GeometryOffset;
		uint GeometryCount;
		uint Colour;
		uint Emissive;

		// -- 16 byte boundary ----
		uint MeshletOffset;
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
		uint MeshletOffset;
		uint MeshletCount;
		uint _padding;

		// -- 16 byte boundary ---
	};

	struct Meshlet
	{
		uint MeshInstanceIdx;
		uint GeometryIdx;
		uint PrimitiveOffset;
	};

	struct Subset
	{
		uint32_t Offset;
		uint32_t Count;
	};

	struct Meshlet_NEW
	{
		uint VertCount;
		uint VertOffset;
		uint PrimCount;
		uint PrimOffset;
	};
	
	struct DrawPacket
	{
		uint InstanceIdx;
		uint GeometryIdx;
	};

	struct VisibilityFillPushConstant
	{
		uint DrawPacketIndex;
	};

	struct MeshletPushConstants
	{
		float4x4 WorldMatrix;
		uint VerticesBufferIdx;
		uint MeshletsBufferIdx;
		uint UniqueVertexIBIdx;
		uint PrimitiveIndicesIdx;

		uint GeometryIdx;
		uint SubsetOffset;
	};

	struct RenderCams
	{
		float4x4 ViewProjection[6];
		// -- 16 byte boundary ---
		uint RtIndex[6];
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
		uint GeometryIndex;
		uint MaterialIndex;
		uint InstancePtrBufferDescriptorIndex;
		uint InstancePtrDataOffset;
		uint DrawFlags;
	};

	struct MiscPushConstants
	{
		float4x4 Transform;
		float4 Colour;
	};

	struct ImagePassPushConstants
	{
		uint Index;
	};

	struct DefferedLightingCSConstants
	{
		uint2 DipatchGridDim; // // Arguments of the Dispatch call
		uint MaxTileWidth; // 8, 16 or 32.
	};

	struct GenerateMipChainPushConstants
	{
		uint TextureInput;
		uint TextureOutput;
		uint ArrayIndex;
		uint _Padding;
		// -- 16 byte boundary ---

		float2 OutputResolution;
		float2 OutputResolutionRcp;
	};

	struct FilterEnvMapPushConstants
	{
		uint TextureInput;
		uint TextureOutput;
		uint ArrayIndex;
		uint NumSamples;
		// -- 16 byte boundary ---

		float2 FilteredResolution;
		float2 FilteredResolutionRcp;
		// -- 16 byte boundary ---

		uint FilterRoughness;
	};

	struct ShaderMeshInstancePointer
	{
		uint Data;

		void Create(uint instanceIndex, uint frustumIndex)
		{
			Data = 0;
			Data |= instanceIndex & 0xFFFFFF;
			Data |= (frustumIndex & 0xF) << 24u;
		};

		uint GetInstanceIndex()
		{
			return Data & 0xFFFFFF;
		}

		uint GetFrustumIndex()
		{
			return (Data >> 24u) & 0xF;
		}
	};

#ifdef __cplusplus
}
#endif
#endif // __PHX_SHADER_INTEROP_STRUCTURES_HLSLI__