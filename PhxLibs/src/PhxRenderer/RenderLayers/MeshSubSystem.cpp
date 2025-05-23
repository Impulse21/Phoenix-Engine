#include "PhxRenderer/PhxRenderer_pch.h"
#include "MeshSubSystem.h"

using namespace phx;
using namespace phx::gfx;

void* phx::gfx::MeshSubSystem::OnPreRender()
{
	return nullptr;
}

void phx::gfx::MeshSubSystem::OnRender(rhi::CommandCtx*, void* )
{
}

void phx::gfx::MeshSubSystem::Finalize()
{
}
