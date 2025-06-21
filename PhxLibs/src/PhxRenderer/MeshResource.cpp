#include "PhxRenderer/PhxRenderer_pch.h"
#include "MeshResource.h"

#include <PhxRhi/PhxRhi.h>

phx::renderer::MeshResource::~MeshResource()
{
	if (gemoetry_buffer.IsValid())
		RHI::DeleteBuffer(gemoetry_buffer);
}
