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
	static const uint LIGHT_TYPE_DIRECTIONAL = 0;
	static const uint LIGHT_TYPE_OMNI = 1;
	static const uint LIGHT_TYPE_SPOT = 2;

	static const uint SHADER_LIGHT_ENTITY_COUNT = 256;

	static const uint MATRIX_COUNT = 128;

	static const uint DRAW_FLAGS_TRANSPARENT = 1 << 0;


	static const uint DDGI_MAX_RAY_COUNT = 512;
	static const uint DDGI_COLOUR_RESOLUTION = 8;
	static const uint DDGI_COLOUR_TEXELS = 1 + DDGI_COLOUR_RESOLUTION + 1; // with norder;
	static const uint DDGI_DEPTH_RESOLUTION = 16;
	static const uint DDGI_DEPTH_TEXELS = 1 + DDGI_DEPTH_RESOLUTION + 1;
	static const float DDGI_KEEP_DISTANCE = 0.1f;

	struct IndirectDrawArgsIndexedInstanced
	{
		uint IndexCount;
		uint InstanceCount;
		uint StartIndex;
		uint VertexOffset;
		uint StartInstance;
	};

	struct IndirectDispatchArgs
	{
		uint GroupCountX;
		uint GroupCountY;
		uint GroupCountZ;
	};

	struct MeshDrawCommand
	{
		uint DrawId;
		IndirectDrawArgsIndexedInstanced Indirect;
	};

	struct MeshletDrawCommand
	{
		uint DrawId;
		IndirectDispatchArgs Indirect;
	};

#ifdef __cplusplus
	static_assert(sizeof(MeshDrawCommand) % sizeof(uint32_t) == 0);
#endif

	// -- Common Buffers END --- 
	struct Light
	{
		float3 Position;
		uint Type8_Flags8_Range16; // <Range_16><flags_8><type_8>

		// -- 16 byte boundary ----
		uint2 Direction16_ConeAngleCos16;
		uint2 ColourPacked; // half4 packed

		// -- 16 byte boundary ----
		float4 ShadowAtlasMulAdd;

		// -- 16 byte boundary ----
		uint AngleOffset16_AngleScale_16;
		uint CubemapFar16_CubemapNear_16;
		uint GlobalMatrixIndex;


#ifndef __cplusplus
		inline uint GetType()
		{
			return Type8_Flags8_Range16 & 0xFF;
		}

		inline uint GetFlags()
		{
			return (Type8_Flags8_Range16 >> 8u) & 0xFF;
		}

		inline float GetRange()
		{
			return f16tof32(Type8_Flags8_Range16 >> 16u);
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
			return f16tof32(Direction16_ConeAngleCos16.y >> 16u);
		}

		inline float4 GetColour()
		{
			float4 retVal;

			retVal.x = f16tof32(ColourPacked.x);
			retVal.y = f16tof32(ColourPacked.x >> 16u);
			retVal.z = f16tof32(ColourPacked.y);
			retVal.w = f16tof32(ColourPacked.y >> 16u);
			return retVal;
		}

		inline float GetAngleScale()
		{
			return f16tof32(AngleOffset16_AngleScale_16);
		}

		inline float GetAngleOffset()
		{
			return f16tof32(AngleOffset16_AngleScale_16 >> 16u);
		}

		inline float GetCubeNearZ()
		{
			return f16tof32(CubemapFar16_CubemapNear_16);
		}

		inline float GetCubeFarZ()
		{
			return f16tof32(CubemapFar16_CubemapNear_16 >> 16u);
		}
		inline bool IsCastingShadows()
		{
			return GlobalMatrixIndex != ~0u;
		}
#else
		inline void SetType(uint type)
		{
			this->Type8_Flags8_Range16 |= type & 0xFF;
		}

		inline void SetFlags(uint flags)
		{
			this->Type8_Flags8_Range16 |= (flags & 0xFF) << 8u;
		}

		inline void SetRange(float value)
		{
			this->Type8_Flags8_Range16 |= DirectX::PackedVector::XMConvertFloatToHalf(value) << 16u;
		}

		inline void SetColor(float4 const& value)
		{
			this->ColourPacked.x |= DirectX::PackedVector::XMConvertFloatToHalf(value.x);
			this->ColourPacked.x |= DirectX::PackedVector::XMConvertFloatToHalf(value.y) << 16u;
			this->ColourPacked.y |= DirectX::PackedVector::XMConvertFloatToHalf(value.z);
			this->ColourPacked.y |= DirectX::PackedVector::XMConvertFloatToHalf(value.w) << 16u;
		}

		inline void SetDirection(float3 const& value)
		{
			this->Direction16_ConeAngleCos16.x |= DirectX::PackedVector::XMConvertFloatToHalf(value.x);
			this->Direction16_ConeAngleCos16.x |= DirectX::PackedVector::XMConvertFloatToHalf(value.y) << 16u;
			this->Direction16_ConeAngleCos16.y |= DirectX::PackedVector::XMConvertFloatToHalf(value.z);
		}

		inline void SetConeAngleCos(float value)
		{
			this->Direction16_ConeAngleCos16.y |= DirectX::PackedVector::XMConvertFloatToHalf(value) << 16u;
		}

		inline void SetAngleScale(float value)
		{
			this->AngleOffset16_AngleScale_16 |= DirectX::PackedVector::XMConvertFloatToHalf(value);
		}

		inline void SetAngleOffset(float value)
		{
			this->AngleOffset16_AngleScale_16 |= DirectX::PackedVector::XMConvertFloatToHalf(value) << 16u;
		}

		inline void SetCubeNearZ(float value)
		{
			this->CubemapFar16_CubemapNear_16 |= DirectX::PackedVector::XMConvertFloatToHalf(value);
		}

		inline void SetCubeFarZ(float value)
		{
			this->CubemapFar16_CubemapNear_16 |= DirectX::PackedVector::XMConvertFloatToHalf(value) << 16u;
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
		uint IndirectEarlyMeshBufferIdx;
		uint IndirectEarlyMeshletBufferIdx;
		uint IndirectLateBufferIdx;

		// -- 16 byte boundary ----
		uint IndirectEarlyTransparentMeshBufferIdx;
		uint IndirectEarlyTransparentMeshletBufferIdx;
		uint IndirectShadowPassMeshBufferIdx;
		uint IndirectShadowPassMeshletBufferIdx;

		// -- 16 byte boundary ----
		uint CulledInstancesBufferUavIdx;
		uint CulledInstancesBufferSrvIdx;
		uint CulledInstancesCounterBufferIdx;
		uint GeometryBoundsBufferIdx;

		// -- 16 byte boundary ----
		uint InstanceCount;
		uint LightCount;
		uint PerLightMeshInstances;
		uint PerLightMeshInstanceCounts;


		uint RT_TlasIndex;
		uint3 _Padding;
		// -- 16 byte boundary ----

		struct DDGI
		{
			uint3 GridDimensions;
			uint ProbCount;

			// -- 16 byte boundary ----
			float3 GridStartPosition;
			uint RTRadianceTexId;

			// -- 16 byte boundary ----
			float3 GridExtents;
			uint RTDirectionDepthTexId;

			// -- 16 byte boundary ----
			float3 GridExtentsRcp;
			uint IrradianceTextureId;

			// -- 16 byte boundary ----

			float3 CellSize;
			float MaxDistance;

			// -- 16 byte boundary ----
			float3 CellSizeRcp;
			uint DepthTextureId;

			// -- 16 byte boundary ----
			uint FrameIndex;

		} DDGI;
	};


	static const uint FRAME_FLAGS_DISABLE_CULL_MESHLET = 1 << 0;
	static const uint FRAME_FLAGS_DISABLE_CULL_FRUSTUM = 1 << 1;
	static const uint FRAME_FLAGS_DISABLE_CULL_OCCLUSION = 1 << 2;
	static const uint FRAME_FLAGS_ENABLE_CLUSTER_LIGHTING = 1 << 3;
	static const uint FRAME_FLAGS_RT_SHADOWS = 1 << 4;
	static const uint FRAME_FLAGS_FLAT_INDIRECT = 1 << 5;


	struct Frame
	{
		uint Flags;
		uint DepthPyramidIndex;
		uint SortedLightBufferIndex;
		uint LightLutBufferIndex;

		// -- 16 byte boundary ----
		uint LightTilesBufferIndex;
		uint LightBufferIdx;
		uint LightMatrixBufferIdx;
		uint ShadowAtlasIdx;

		// -- 16 byte boundary ----
		uint2 ShadowAtlasRes;
		float2 ShadowAtlasResRCP;

		// -- 16 byte boundary ----
		uint2 Resolution;
		uint2 _Padding;

		// -- 16 byte boundary ----
		Scene SceneData;
	};

	struct Camera
	{
		float4x4 Proj;

		// -- 16 byte boundary ----
		float4x4 View;

		// -- 16 byte boundary ----
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
		float4 PlanesWS[6];

		// -- 16 byte boundary ----
#ifndef __cplusplus
		inline float3 GetPosition()
		{
			return ViewInv[3].xyz;
		}
		inline float GetZNear()
		{
			return -Proj._43 / Proj._33;
		}
		inline float GetZFar()
		{
			return -Proj._43 / (Proj._33 - 1.0f);
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
		bool CastShadows; // TODO: remove once proper render buckets are sorted out
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
		uint MeshletPrimtiveOffset;
		// -- 16 byte boundary ---

		uint MeshletUniqueVertexIBOffset;
		uint DrawFlags;
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
		uint DrawBufferMeshIdx;
		uint DrawBufferMeshletIdx;
		uint DrawBufferTransparentMeshIdx;
		uint DrawBufferTransparentMeshletIdx;
		uint CulledDataSRVIdx;
		uint CulledDataCounterSrcIdx;
		bool IsLatePass;
	};

	struct FillPerLightListConstants
	{
		bool UseMeshlets;
		uint PerLightMeshCountsUavIdx;
		uint PerLightMeshInstancesUavIdx;
	};

	struct FillLightDrawBuffers
	{
		bool UseMeshlets;
		uint DrawBufferIdx;
		uint DrawBufferCounterIdx;
	};

	struct DepthPyrmidPushConstnats
	{
		uint inputTextureIdx;
		uint outputTextureIdx;
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

	struct ShadowCams
	{
		float4x4 ViewProjection[6];
	};

	struct DDGIPushConstants
	{
		uint NumRays;
		uint FirstFrame;
		float BlendSpeed;
		uint GiBoost;
		float4x4 RandRotation;
	};
#ifdef __cplusplus
}
#endif
#endif // __PHX_SHADER_INTEROP_STRUCTURES_HLSLI__