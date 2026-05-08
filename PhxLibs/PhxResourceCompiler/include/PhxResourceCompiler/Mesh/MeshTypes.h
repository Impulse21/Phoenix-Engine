#pragma once

#include <vector>
#include <string>

#include <PhxCore/Span.h>
#include <PhxCore/Math.h>
#include <PhxCore/IO/MemoryRegion.h>

#include <PhxRenderer/Shaders/ShaderInterop.h>

namespace phx::resource::compiler
{
	namespace PSOFlags
	{
		enum : uint16_t
		{
			kAlphaBlend = BIT(1),
			kAlphaTest = BIT(2),
			kTwoSided = BIT(3),
		};
	}

	struct VertexBufferStreams
	{
		std::unique_ptr<std::vector<hlslpp::float3>> positions;   
		std::unique_ptr<std::vector<hlslpp::float3>> normals;     
		std::unique_ptr<std::vector<hlslpp::float2>> texCoords_0;
		std::unique_ptr<std::vector<hlslpp::float2>> texCoords_1;
		std::unique_ptr<std::vector<hlslpp::float4>> tangents;
		std::unique_ptr<std::vector<hlslpp::float3>> colour;
		std::unique_ptr<std::vector<hlslpp::uint4>> joints_0;
		std::unique_ptr<std::vector<hlslpp::float4>> weights_0;
	};

	struct RawSubMesh
	{
		VertexBufferStreams vertex_streams;
		std::unique_ptr<std::vector<uint32_t>> indices;

		math::BoundingSphere bounds_ls;		// local space bounds
		math::AxisAlignedBox bbox_ls;		// local space AABB
		std::string material_id;

		union
		{
			uint32_t hash;
			struct {
				uint32_t pso_flags : 16;
				uint32_t index_32 : 1;
				uint32_t material_index : 15;
			};
		};
	};

	struct RawMesh
	{
		std::vector<RawSubMesh> sub_meshes;	
	};

    struct BakedMesh
    {
        // The single, interleaved vertex buffer for ALL submeshes.
        MemoryBuffer vertex_buffer;
		MemoryBuffer index_buffer;

        struct SubMeshView
        {
            uint32_t index_count;
            uint32_t index_offset;
            uint32_t vertex_offset;

            // Bounding box, material ID, etc.
            std::string material_id;
        };

		math::AxisAlignedBox bbox_ls;
		math::BoundingSphere bounds_ls;
        std::vector<SubMeshView> sub_meshes;
    };
}
