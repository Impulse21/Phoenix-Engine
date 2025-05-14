#include "PhxRenderer/PhxRenderer_pch.h"
#include "ForwardRenderer.h"

#include <PhxWorld/World.h>
#include <PhxWorld/Entity.h>
#include <PhxWorld/WorldComponents.h>

#include <PhxResource/ResourceManger.h>

#include <PhxRhi/RHICore.h>

#define ENABLE_ENTT_CALLBACKS false
using namespace phx;
using namespace phx::gfx;


namespace
{
#if ENABLE_ENTT_CALLBACKS
	void Testing(entt::registry& , entt::entity )
	{
	}
#endif
}
void phx::gfx::ForwardRenderer::RegisterObserver(phx::World& world)
{
#if ENABLE_ENTT_CALLBACKS
	world.GetRegistry().on_construct<MeshComponent>().connect<&Testing>();
#else
	// This observer tracks when MeshComponent is constructed on any entity
	m_observer.connect(
		world.GetRegistry(),
		entt::collector
		.group<MeshComponent>());  // group just ensures it's on construct
#endif
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

		auto& meshComp = entity.GetComponent<MeshComponent>();
		PHX_CORE_INFO("Mesh Component was added {0}", meshComp.Mesh.c_str());
		RefCountPtr<IResource> resource = ResourceManger::Get(meshComp.Mesh.c_str());
		if (resource)
		{
			auto& renderComponent = entity.AddComponent<RenderMeshComponent>();
			renderComponent.MeshResource = resource;
		}
	}

	m_observer.clear();

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
