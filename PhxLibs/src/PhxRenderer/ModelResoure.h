#pragma once

#include <PhxCore/IO/MemoryRegion.h>
#include <PhxRhi/RHICommon.h>

#include <PhxResource/Resource.h>

namespace phx::renderer
{
	struct ModelMetadata
	{
		uint32_t geometry_bufer_size;
	};

	struct Mesh
	{
		std::array<float, 4> bounds;           // A bounding sphere
		uint32_t             vb_offset;         // BufferLocation - Buffer.GpuVirtualAddress
		uint32_t             vb_size;           // SizeInBytes
		uint32_t             ib_offset;         // BufferLocation - Buffer.GpuVirtualAddress
		uint32_t             ib_size;           // SizeInBytes
		uint8_t              ib_format;         // DXGI_FORMAT
		uint16_t             mesh_cbv;          // Index of mesh constant buffer
		uint16_t             material_cbv;      // Index of material constant buffer
		uint16_t             pso_flags;         // Flags needed to request a PSO
		uint16_t             pso;               // Index of pipeline state object
		uint16_t             num_joints;        // Number of skeleton joints when skinning
		uint16_t             start_joint;       // Flat offset to first joint index
		uint16_t             num_draws;         // Number of draw groups

		struct Draw
		{
			uint32_t prim_count;    // Number of indices = 3 * number of triangles
			uint32_t start_index;   // Offset to first index in index buffer
			uint32_t base_vertex;   // Offset to first vertex in vertex buffer
		};
		Draw draw[1];               // Actually 1 or more draws
	};

	struct ModelResoure final : public Resource
	{
		PHX_DECLARE_RESOURCE(ModelResoure)

		~ModelResoure() override;
		struct CpuData
		{
			float		bounding_sphere[4];
			float		bounding_box[4];
			Mesh*		meshes;
			uint32_t	num_meshes;
		};

		phx::MemoryBuffer cpu_data_buffer;
		TypedView<CpuData> cpu_data;

		RHI::GpuBufferHandle gemoetry_buffer;
	};

}

