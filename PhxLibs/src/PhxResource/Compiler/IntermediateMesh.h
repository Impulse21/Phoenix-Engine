#pragma once

#include <vector>
#include <string>

#include <PhxCore/Span.h>
#include <PhxCore/Math.h>

#include <PhxRenderer/shaders/ShaderInterop.h>


namespace phx::compiler
{
	struct IntermediateSubMesh
	{
		std::vector<hlslpp::float3> positions;   
		std::vector<hlslpp::float3> normals;     
		std::vector<hlslpp::float2> texCoords_0;
		std::vector<hlslpp::float2> texCoords_1;
		std::vector<hlslpp::float4> tangents;
		std::vector<hlslpp::float3> colour;
		std::vector<hlslpp::uint4> joints_0;
		std::vector<hlslpp::float4> weights_0;

		std::vector<uint32_t> indices;

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

    struct IntermediateMesh
    {
        [[nodiscard]] static IntermediateMesh Create(Span<IntermediateSubMesh> sub_meshes);
        
        // The single, interleaved vertex buffer for ALL submeshes.
        std::unique_ptr<std::byte[]> vertex_buffer;
		size_t vertex_buffer_size;

		std::unique_ptr<std::byte[]> index_buffer;
		size_t index_buffer_size;

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
