#pragma once

#include "PhxRenderer/shaders/ShaderInterop.h"
#include <PhxResource/ResourceManger.h>

namespace phx::renderer
{
	struct MeshCpuMetadata
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

	class MeshResource final : public RefCounter<IResource>
	{
		friend class MeshResourceHandler;
	public:
		bool IsLoaded() const { return false; }
		FileHandle GetFileHandle() const { return {}; }

	};
}


