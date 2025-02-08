#pragma once

#include "PhxRenderer/shaders/ShaderInterop.h"

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
			uint32_t PrimCount;
			uint32_t StartIndex;
			uint32_t IndexCount;
		};
		DrawInfo Draw[1];
	};

	class MeshResource
	{
	};
}

