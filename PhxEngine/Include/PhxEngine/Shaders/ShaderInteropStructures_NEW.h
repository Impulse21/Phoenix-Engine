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

	struct Scene
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

	struct ObjectInstance
	{
		float4x4 WorldMatrix;
		// -- 16 byte boundary ----

		uint GeometryOffset;
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
			float4 refVal;

			refVal.x = (float)((EmissiveColourPacked >> 0) & 0xFF) / 255.0f;
			refVal.y = (float)((EmissiveColourPacked >> 8) & 0xFF) / 255.0f;
			refVal.z = (float)((EmissiveColourPacked >> 16) & 0xFF) / 255.0f;
			refVal.w = (float)((EmissiveColourPacked >> 24) & 0xFF) / 255.0f;

			return refVal;
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
		uint _padding;

		// -- 16 byte boundary ---
	};

	struct Meshlet
	{
		uint VertCount;
		uint VertOffset;
		uint PrimCount;
		uint PrimOffset;
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

			return refVal;
		}

		float4 GetTanget()
		{
			float4 retVal;

			retVal.x = (float)((Tanget >> 0) & 0xFF) / 127.0f;
			retVal.y = (float)((Tanget >> 8) & 0xFF) / 127.0f;
			retVal.z = (float)((Tanget >> 16) & 0xFF) / 127.0f;
			retVal.w = (float)((Tanget >> 24) & 0xFF) / 127.0f;

			return refVal;
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

#ifdef __cplusplus
}
#endif
#endif // __PHX_SHADER_INTEROP_STRUCTURES_HLSLI__