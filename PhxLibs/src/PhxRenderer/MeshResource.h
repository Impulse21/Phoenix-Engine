#pragma once

#include <PhxCore/IO/MemoryRegion.h>
#include <PhxRhi/RHITypes.h>
#include <PhxResource/ResourceManger.h>

#include "PhxRenderer/shaders/ShaderInterop.h"

namespace phx::renderer
{
	// This affects the resource modify qith care

	struct MeshMetadata
	{
		uint32_t GeometryBufferSize;
		uint32_t VertexBufferOffset;
	};

	class MeshResource final : public RefCounter<IResource>
	{
		friend class MeshResourceHandler;
	public:
		~MeshResource();

		bool IsLoaded() const { return false; }
		StreamFileHandle GetFileHandle() const { return {}; }

	public:
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

	private:
		phx::MemoryRegion<CpuData> m_cpuData;
		rhi::GpuBufferHandle m_geometryBuffer;
		std::atomic_uint8_t m_status = 0xFF;
	};
}
