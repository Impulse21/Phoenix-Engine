#include "PhxRenderer/PhxRenderer_pch.h"
#include "MeshResource.h"

#include <PhxRhi/PhxRhi.h>

phx::renderer::MeshResource::~MeshResource()
{
	RHI::DeleteBuffer(packed_mesh_buffer);
}
