#include "PhxRenderer/PhxRenderer_pch.h"
#include "MeshResource.h"

#include <PhxRhi/PhxRhi.h>

phx::renderer::MeshResource::~MeshResource()
{
	rhi::DeleteBuffer(packed_mesh_buffer);
}
