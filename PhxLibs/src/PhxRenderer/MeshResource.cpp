#include "PhxRenderer/PhxRenderer_pch.h"
#include "MeshResource.h"

#include <PhxRhi/GfxDevice.h>

phx::renderer::MeshResource::~MeshResource()
{
	if (m_geometryBuffer.IsValid())
		rhi::GetDevice().DeleteBuffer(m_geometryBuffer);
}
