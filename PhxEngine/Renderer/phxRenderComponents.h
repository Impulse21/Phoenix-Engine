#pragma once

#include "RHI/PhxRHI.h"
namespace phx
{
	struct RenderableComponent
	{
		rhi::BufferHandle VertexBuffer;
		rhi::BufferHandle IndexBuffer;
	};
}