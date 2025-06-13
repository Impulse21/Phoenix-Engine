#pragma once

#include <PhxCore/IO/MemoryRegion.h>
#include <PhxRhi/RHICommon.h>

#include <PhxResource/Resource.h>

namespace phx::renderer
{
	// This affects the resource modify qith care

	struct MeshMetadata
	{
		uint32_t GeometryBufferSize;
		uint32_t VertexBufferOffset;
	};

	struct MeshResource final : public Resource
	{
		PHX_DECLARE_RESOURCE(MeshResource)

		struct CpuData
		{
			float Bounds[4];
			uint32_t VbOffset;
			uint32_t VbSize;
			uint32_t IbOffset;
			uint32_t IbSize;
			uint8_t IbFormat;
			uint16_t NumDraws;

			struct DrawInfo
			{
				uint32_t IndexCount;
				uint32_t StartIndex;
				uint32_t BaseVertex;
			};
			DrawInfo Draw[1];
		};

		phx::MemoryBuffer cpu_data_buffer;
		TypedView<CpuData> cpu_data;

		rhi::GpuBufferHandle gemoetry_buffer;

		~MeshResource();
	};
}
