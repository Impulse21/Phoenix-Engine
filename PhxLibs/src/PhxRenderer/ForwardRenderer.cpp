#include "PhxRenderer/PhxRenderer_pch.h"
#include "ForwardRenderer.h"

#include <PhxRhi/RHICore.h>

using namespace phx;
using namespace phx::gfx;

void ForwardRenderer::Finalize()
{
	for (auto& system : m_renderSystems)
		system->Finalize();
}

void phx::gfx::ForwardRenderer::OnPreRender()
{
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
