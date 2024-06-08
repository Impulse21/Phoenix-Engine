#include "pch.h"
#include "Editor.h"

#include "phxBaseInclude.h"
#include "Graphics/phxGfxCore.h"

void phx::editor::Editor::OnStartup()
{
}

void phx::editor::Editor::OnShutdown()
{
}

void phx::editor::Editor::OnPreRender(PHX_UNUSED Subflow* subflow)
{
}

void phx::editor::Editor::OnRender(PHX_UNUSED Subflow* subflow)
{
	
	phx::gfx::SubmitFrame();
}

void phx::editor::Editor::OnUpdate(PHX_UNUSED Subflow* subflow)
{
}
