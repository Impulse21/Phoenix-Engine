#pragma once
#include <cstdint>

#include "Core/phxPlatform.h"
#include "phxGfxEnums.h"

namespace phx::gfx
{
	struct DeviceCapabilities
	{
		union
		{
			uint8_t Flags = 0;
			struct
			{
				uint8_t RTVTArrayIndexWithoutGS : 1;
				uint8_t RayTracing : 1;
				uint8_t RenderPass : 1;
				uint8_t RayQuery : 1;
				uint8_t VariableRateShading : 1;
				uint8_t MeshShading : 1;
				uint8_t CreateNoteZeroed : 1;
				uint8_t Bindless : 1;
			};
		};
	};

	struct ContextDesc
	{
		core::WindowHandle Window = NULL;
		int WindowWidth = 1;
		int WindowHeight = 1;
		Format BackBufferFormat = Format::RGBA8_UNORM;
		uint32_t BackBufferCount = 2;

		union
		{
			uint8_t Flags;
			struct
			{
				uint8_t AllowTearing : 1;
				uint8_t EnableHdr : 1;
				uint8_t RequresRaytracing : 6;
			};
		};
	};
}