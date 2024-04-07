#include <PhxEngine/Core/EntryPoint.h>
#include <PhxEngine/Core/Application.h>

#include "Renderer.h"

#include "EditorLayer.h"

#include <imgui.h>
#include <imgui_internal.h>

using namespace PhxEngine;

namespace PhxEditor
{
	class PhxEditor : public PhxEngine::Application
	{
	public:
		PhxEditor()
		{
			this->PushOverlay<EditorLayer>();
		}
	};
}

PhxEngine::Application* PhxEngine::CreateApplication(ApplicationCommandLineArgs args)
{
	return new PhxEditor::PhxEditor();
}

