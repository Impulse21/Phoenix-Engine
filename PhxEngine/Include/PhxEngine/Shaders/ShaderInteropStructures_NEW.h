#ifndef __PHX_SHADER_INTEROP_STRUCTURES_NEW_HLSLI__
#define __PHX_SHADER_INTEROP_STRUCTURES_NEW_HLSLI__

#ifdef __cplusplus

#include "ShaderInterop.h"

#else 

#include "../Include/PhxEngine/Shaders/ShaderInterop.h"
#include "PackHelpers.hlsli"
#endif 

#ifdef __cplusplus

#include <Directxpackedvector.h>

namespace Shader::New
{
#endif
	static const uint ENTITY_TYPE_DIRECTIONALLIGHT = 0;
	static const uint ENTITY_TYPE_OMNILIGHT = 1;
	static const uint ENTITY_TYPE_SPOTLIGHT = 2;

	static const uint SHADER_LIGHT_ENTITY_COUNT = 256;

	static const uint MATRIX_COUNT = 128;
	struct IndirectDrawArgsIndexedInstanced
	{
		uint32_t IndexCount;
		uint32_t InstanceCount;
		uint32_t StartIndex;
		uint32_t VertexOffset;
		uint32_t StartInstance;
	};

	struct IndirectDispatchArgs
	{
		uint32_t GroupCountX;
		uint32_t GroupCountY;
		uint32_t GroupCountZ;
	};

	struct MeshDrawCommand
	{
		uint DrawId;
		IndirectDrawArgsIndexedInstanced Indirect;
		IndirectDispatchArgs IndirectMS;
	};

#ifdef __cplusplus
	static_assert(sizeof(MeshDrawCommand) % sizeof(uint32_t) == 0);
#endif

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
			float4 retVal;

			retVal.x = (float)((ColorPacked >> 0) & 0xFF) / 255.0f;
			retVal.y = (float)((ColorPacked >> 8) & 0xFF) / 255.0f;
			retVal.z = (float)((ColorPacked >> 16) & 0xFF) / 255.0f;
			retVal.w = (float)((ColorPacked >> 24) & 0xFF) / 255.0f;

			return retVal;
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

	struct Scene
	{
		uint ObjectBufferIdx;
		uint GeometryBufferIdx;
		uint MaterialBufferIdx;
		uint GlobalVertexBufferIdx;

		// -- 16 byte boundary ----
		uint GlobalIndexBufferIdx;
		uint MeshletCullDataBufferIdx;
		uint MeshletBufferIdx;
		uint MeshletPrimitiveIdx;

		// -- 16 byte boundary ----
		uint UniqueVertexIBIdx;
		uint IndirectEarlyBufferIdx;
		uint IndirectLateBufferIdx;
		uint IndirectCullBufferIdx;
	};


	static const uint FRAME_FLAGS_DISABLE_CULL_MESHLET = 1 << 0;

	struct Frame
	{
		uint Flags;
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
		float4 Planes[6];

#ifndef __cplusplus
		inline float3 GetPosition()
		{
			return ViewInv[3].xyz;
		}
#endif 
	};

	struct ObjectInstance
	{
		float4x4 WorldMatrix;
		// -- 16 byte boundary ----

		uint GeometryIndex;
		uint Colour;
		uint Emissive;
		uint MeshletOffset;

		// -- 16 byte boundary ----
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

		uint Flags;
		uint EmissiveColourPacked;
		uint2 _Padding;

#ifndef __cplusplus
		inline float4 GetEmissiveColour()
		{
			float4 retVal;

			retVal.x = (float)((EmissiveColourPacked >> 0) & 0xFF) / 255.0f;
			retVal.y = (float)((EmissiveColourPacked >> 8) & 0xFF) / 255.0f;
			retVal.z = (float)((EmissiveColourPacked >> 16) & 0xFF) / 255.0f;
			retVal.w = (float)((EmissiveColourPacked >> 24) & 0xFF) / 255.0f;

			return retVal;
		}
#endif
	};

	struct Geometry
	{
		uint MaterialIndex;
		uint NumIndices;
		uint NumVertices;
		uint IndexByteOffset;

		// -- 16 byte boundary ---
		uint VertexBufferIndex;
		uint PositionOffset;
		uint TexCoordOffset;
		uint NormalOffset;

		// -- 16 byte boundary ---
		uint TangentOffset;

		// TODO: I am here
		uint MeshletOffset;
		uint MeshletCount;
		uint MeshletPrimtiveOffset;
		uint MeshletUniqueVertexIBOffset;
		// -- 16 byte boundary ---
	};

	struct Meshlet
	{
		uint VertCount;
		uint VertOffset;
		uint PrimCount;
		uint PrimOffset;
	};

	struct CullData
	{
		float4 BoundingSphere; // xyz = center, w = radius
		uint NormalCone;     // axis : 24, w = -cos(a + 90)
		float ApexOffset;     // apex = center - axis * offset

#ifndef __cplusplus
		float4 UnpackCone()
		{
			float4 v;
			v.x = float((NormalCone >> 0) & 0xFF);
			v.y = float((NormalCone >> 8) & 0xFF);
			v.z = float((NormalCone >> 16) & 0xFF);
			v.w = float((NormalCone >> 24) & 0xFF);

			v = v / 255.0;
			v.xyz = v.xyz * 2.0 - 1.0;

			return v;
		}
		bool IsConeDegenerate()
		{
			return (NormalCone >> 24) == 0xff;
		}
#endif
	};

	struct MeshletVertexPositions
	{
		float3 Position;
		uint _Padding;
	};

	struct MeshletPackedVertexData
	{
		uint Normal; 
		uint Tanget; 
		uint TexCoords;
		uint _padding;

#ifndef __cplusplus

		float4 GetNormal()
		{
			float4 retVal;

			retVal.x = (float)((Normal >> 0) & 0xFF) / 127.0f;
			retVal.y = (float)((Normal >> 8) & 0xFF) / 127.0f;
			retVal.z = (float)((Normal >> 16) & 0xFF) / 127.0f;
			retVal.w = (float)((Normal >> 24) & 0xFF) / 127.0f;

			return retVal;
		}

		float4 GetTanget()
		{
			float4 retVal;

			retVal.x = (float)((Tanget >> 0) & 0xFF) / 127.0f;
			retVal.y = (float)((Tanget >> 8) & 0xFF) / 127.0f;
			retVal.z = (float)((Tanget >> 16) & 0xFF) / 127.0f;
			retVal.w = (float)((Tanget >> 24) & 0xFF) / 127.0f;

			return retVal;
		}

		float2 GetTexCoord()
		{
			return UnpackHalf2(TexCoords);
		}
#else
		void SetNormal(float4 normal)
		{
			Normal = 0;
			Normal |= (uint8_t)((normal.x + 1.0f) * 127.0) << 0;
			Normal |= (uint8_t)((normal.x + 1.0f) * 127.0) << 8;
			Normal |= (uint8_t)((normal.x + 1.0f) * 127.0) << 16;
			Normal |= (uint8_t)((normal.x + 1.0f) * 127.0) << 24;
		}

		void SetTangent(float4 tangent)
		{
			Tanget = 0;
			Tanget |= (uint8_t)((tangent.x + 1.0f) * 127.0) << 0;
			Tanget |= (uint8_t)((tangent.x + 1.0f) * 127.0) << 8;
			Tanget |= (uint8_t)((tangent.x + 1.0f) * 127.0) << 16;
			Tanget |= (uint8_t)((tangent.x + 1.0f) * 127.0) << 24;
		}

		void SetTexCoord(float2 texCoord)
		{
			DirectX::PackedVector::HALF x = DirectX::PackedVector::XMConvertFloatToHalf(texCoord.x);
			DirectX::PackedVector::HALF y = DirectX::PackedVector::XMConvertFloatToHalf(texCoord.y);
			TexCoords = x || y << 16u;
		}
#endif
	};

	struct CullPushConstants
	{

	};

	struct DefferedLightingCSConstants
	{
		uint2 DipatchGridDim; // // Arguments of the Dispatch call
		uint MaxTileWidth; // 8, 16 or 32.
	};

	struct GeometryPushConstant
	{
		uint DrawId;
	};

#ifdef __cplusplus
}
#endif
#endif // __PHX_SHADER_INTEROP_STRUCTURES_HLSLI__