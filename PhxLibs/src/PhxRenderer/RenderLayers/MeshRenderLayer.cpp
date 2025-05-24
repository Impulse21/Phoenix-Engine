#include "PhxRenderer/PhxRenderer_pch.h"
#include "MeshRenderLayer.h"

#include <PhxWorld/World.h>
#include <PhxRenderer/RenderSystem.h>

using namespace phx;
using namespace phx::gfx;

void* phx::gfx::MeshRenderLayer::PreRender(phx::World&, View const& , RenderPass)
{
	return nullptr;
}

void phx::gfx::MeshRenderLayer::Render(RenderPass /*renderPass*/, void* /*cachedData*/)
{
}

void phx::gfx::MeshRenderLayer::Finalize()
{
}
