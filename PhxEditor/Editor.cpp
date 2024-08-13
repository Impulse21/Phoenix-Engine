#include "pch.h"
#include "Editor.h"

#include "phxBaseInclude.h"
#include "RHI/phxRHI.h"

#include "Renderer/phxRendererForward.h"
#include "Renderer/phxRenderSystem.h"

#include "World/phxWorld.h"

void phx::editor::Editor::OnStartup()
{
	this->m_renderer = std::make_unique<RendererForward>();
	this->m_renderer->RegisterRenderSystem<RenderSystemMesh>();

	this->m_world = std::make_unique<World>();

	// attach a cube

}

void phx::editor::Editor::OnShutdown()
{
}

void phx::editor::Editor::OnPreRender(PHX_UNUSED Subflow* subflow)
{
	this->m_renderer->CacheRenderData();
}

void phx::editor::Editor::OnRender(PHX_UNUSED Subflow* subflow)
{
	
}

void phx::editor::Editor::OnUpdate(PHX_UNUSED Subflow* subflow)
{
}
