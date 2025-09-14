#pragma once

#include <PhxCore/IO/MemoryRegion.h>
#include <PhxRhi/RHICommon.h>

#include <PhxResource/Resource.h>

namespace phx::renderer
{
	// This affects the resource modify with care

	struct MeshMetadata
	{
		uint32_t geometry_bufer_size;
		uint32_t vertex_buffer_size;
	};

	struct MeshResource final : public Resource
	{
		PHX_DECLARE_RESOURCE(MeshResource)

		struct CpuData
		{
			float		bounds[4];
			uint32_t	vb_offset;
			uint32_t	vb_size;
			uint32_t	ib_offset;
			uint32_t	ib_size;
			uint8_t		ib_format;
			uint16_t	num_joints;        // Number of skeleton joints when skinning
			uint16_t	start_joint;       // Flat offset to first joint index
			uint16_t	num_draws;

			struct DrawInfo
			{
				uint32_t prim_count;
				uint32_t start_index;
				uint32_t base_vertex;
			};
			DrawInfo Draw[1];
		};

		phx::MemoryBuffer cpu_data_buffer;
		TypedView<CpuData> cpu_data;

		RHI::GpuBufferHandle gemoetry_buffer;

		~MeshResource();
	};
}
