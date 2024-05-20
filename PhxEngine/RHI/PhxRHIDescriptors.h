#pragma once

#include "Core/CommonInclude.h"
#include "Core/Platform.h"
#include "PhxRHIEnums.h"

namespace phx::rhi
{
	struct DeviceCapabilities
	{
		union
		{
			uint8 Flags = 0;
			struct
			{
				uint8 RTVTArrayIndexWithoutGS : 1;
				uint8 RayTracing : 1;
				uint8 RenderPass : 1;
				uint8 RayQuery : 1;
				uint8 VariableRateShading : 1;
				uint8 MeshShading : 1;
				uint8 CreateNoteZeroed : 1;
				uint8 Bindless : 1;
			};
		};
	};

	struct RHIDesc
	{
		core::WindowHandle Window = NULL;
		int WindowWidth = 1;
		int WindowHeight = 1;
		Format BackBufferFormat = Format::RGBA8_UNORM;
		uint32 BackBufferCount = 2;

		union
		{
			uint8 Flags;
			struct
			{
				uint8 AllowTearing	: 1;
				uint8 EnableHdr		: 1;
			};
		};
	};
}