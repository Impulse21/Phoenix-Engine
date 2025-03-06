#pragma once

#include "PhxRenderer/shaders/ShaderInterop.h"
#include <PhxResource/ResourceManger.h>

namespace phx::renderer
{
	// This affects the resource modify qith care
	namespace data
	{
		struct MeshMetadata
		{
			float Bounds[4];
			uint32_t VbOffset;
			uint32_t VbSize;
			uint32_t IbOffset;
			uint32_t IbSize;
			uint8_t IbFormat;
			uint16_t numGeometry;
			struct GeometryData
			{
				phx::StringHash MaterialId = {};
				uint32_t IndexOffset = 0;
				uint32_t IndexCount = 0;
			};

			GeometryData Geo[1];
		};
	}

	class MeshResource final : public RefCounter<IResource>
	{
		friend class MeshResourceHandler;
	public:
		bool IsLoaded() const { return false; }
		StreamFileHandle GetFileHandle() const { return {}; }

	private:
#if false
		float m_bounds[4];
		uint32_t m_vbOffset;
		uint32_t m_vbSize;
		uint32_t m_ibOffset;
		uint32_t m_ibSize;
		uint8_t m_ibFormat;
		uint16_t m_numDraws;

		struct DrawInfo
		{
			uint32_t IndexCount;
			uint32_t StartIndex;
			uint32_t BaseVertex;
		};
		std::vector<DrawInfo> Draws;
#endif
	};
}


