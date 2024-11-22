#pragma once

#include <string>
#include <vector>

#include "phxMath.h"

namespace phxed
{
	constexpr size_t kNumTextures = 5;


	struct Mesh
	{
		float    bounds[4];     // A bounding sphere
		uint32_t vbOffset;      // BufferLocation - Buffer.GpuVirtualAddress
		uint32_t vbSize;        // SizeInBytes
		uint32_t vbDepthOffset; // BufferLocation - Buffer.GpuVirtualAddress
		uint32_t vbDepthSize;   // SizeInBytes
		uint32_t ibOffset;      // BufferLocation - Buffer.GpuVirtualAddress
		uint32_t ibSize;        // SizeInBytes
		uint8_t  vbStride;      // StrideInBytes
		uint8_t  ibFormat;      // DXGI_FORMAT
		uint16_t meshCBV;       // Index of mesh constant buffer
		uint16_t materialCBV;   // Index of material constant buffer
		uint16_t srvTable;      // Offset into SRV descriptor heap for textures
		uint16_t samplerTable;  // Offset into sampler descriptor heap for samplers
		uint16_t psoFlags;      // Flags needed to request a PSO
		uint16_t pso;           // Index of pipeline state object
		uint16_t numJoints;     // Number of skeleton joints when skinning
		uint16_t startJoint;    // Flat offset to first joint index
		uint16_t numDraws;      // Number of draw groups

		struct Draw
		{
			uint32_t primCount;   // Number of indices = 3 * number of triangles
			uint32_t startIndex;  // Offset to first index in index buffer 
			uint32_t baseVertex;  // Offset to first vertex in vertex buffer
		};
		Draw draw[1];           // Actually 1 or more draws
	};

	// Unaligned mirror of MaterialConstants
	struct MaterialConstantData
	{
		float BaseColour[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		float EmissiveColour[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		float Metalness = 1.0f;
		float Rougness = 1.0f;
		union
		{
			uint32_t Flags;
			struct
			{
				// UV0 or UV1 for each texture
				uint32_t BaseColorUV : 1;
				uint32_t MetallicRoughnessUV : 1;
				uint32_t OcclusionUV : 1;
				uint32_t EmissiveUV : 1;
				uint32_t NormalUV : 1;

				// Three special modes
				uint32_t TwoSided : 1;
				uint32_t AlphaTest : 1;
				uint32_t AlphaBlend : 1;

				uint32_t _pad : 8;

				uint32_t AlphaCutoff : 16; // half float
			};
		};
	};

	// Used at load time to construct descriptor tables
	struct MaterialTextureData
	{
		uint16_t StringIdx[kNumTextures];
		uint32_t AddressModes;
	};

	struct ModelData
	{
		phx::math::Sphere BoundingSphere;
		phx::math::AABB BoundingBox;
		std::vector<uint8_t> GeometryData;
		std::vector<std::string> TextureNames;
		std::vector<MaterialConstantData> MaterialConstants;
		std::vector<MaterialTextureData> MaterialTextures;
		std::vector<Mesh*> Meshes;
		std::vector<uint8_t> TextureOptions;
	};

	class ModelImporter
	{
	public:
		virtual ~ModelImporter() = default;

		virtual bool Import(std::string const& fileName, ModelData& outModel) = 0;
	};

}