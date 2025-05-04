#include "PhxRenderer/PhxRenderer_pch.h"
#include "ForwardRenderer.h"

#include <PhxWorld/World.h>
#include <PhxWorld/Entity.h>
#include <PhxWorld/WorldComponents.h>

#include <PhxRhi/RHICore.h>

using namespace phx;
using namespace phx::gfx;

void phx::gfx::ForwardRenderer::RegisterObserver(phx::World& world)
{
	// This observer tracks when MeshComponent is constructed on any entity
	m_observer.connect(
		world.GetRegistry(),
		entt::collector
		.group<MeshComponent>());  // group just ensures it's on construct
}

void ForwardRenderer::Finalize()
{
	for (auto& system : m_renderSystems)
		system->Finalize();
}

void phx::gfx::ForwardRenderer::OnPreRender(World& world)
{
	for (entt::entity entityId : m_observer)
	{
		Entity entity = { entityId, &world };
		if (entity.HasComponent<RenderMeshComponent>())
			continue;

		auto& renderComponent = entity.AddComponent<RenderMeshComponent>();
		// TODO: Set Resource
	}

	for (size_t i = 0; i < m_renderSystems.size(); i++)
	{
		if (m_passMasks[i] & ForwardRenderPasses::GuiPass)
			m_cachedData[i] = m_renderSystems[i]->OnPreRender();
	}
}

void phx::gfx::ForwardRenderer::OnRender()
{
	rhi::CommandCtx* ctx = rhi::BeginCommnadCtx();
	ctx->RenderPassBegin();
	for (size_t i = 0; i < m_renderSystems.size(); i++)
	{
		if (m_passMasks[i] & ForwardRenderPasses::GuiPass)
			m_renderSystems[i]->OnRender(ctx, m_cachedData[i]);
	}

	ctx->RenderPassEnd();
}
