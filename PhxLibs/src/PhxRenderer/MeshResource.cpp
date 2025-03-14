#include "PhxRenderer/PhxRenderer_pch.h"
#include "MeshResource.h"

#include <PhxRhi\RHICore.h>

phx::renderer::MeshResource::~MeshResource()
{
	if (m_geometryBuffer.IsValid())
		rhi::DeleteBuffer(m_geometryBuffer);
}
