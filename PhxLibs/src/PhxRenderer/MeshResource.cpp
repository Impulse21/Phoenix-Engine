#include "PhxRenderer/PhxRenderer_pch.h"
#include "MeshResource.h"

#include <PhxRhi/GfxDevice.h>

phx::renderer::MeshResource::~MeshResource()
{
	if (gemoetry_buffer.IsValid())
		rhi::GetDevice().DeleteBuffer(gemoetry_buffer);
}
