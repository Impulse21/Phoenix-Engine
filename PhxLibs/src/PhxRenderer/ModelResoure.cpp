#include "PhxRenderer/PhxRenderer_pch.h"
#include "ModelResoure.h"

#include <PhxRhi/PhxRhi.h>

phx::renderer::ModelResoure::~ModelResoure()
{
	if (gemoetry_buffer.IsValid())
		RHI::DeleteBuffer(gemoetry_buffer);
}
