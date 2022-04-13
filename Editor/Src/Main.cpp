
#include <memory>

#include <DirectXMath.h>
#include <PhxEngine/App/EntryPoint.h>

#include <PhxEngine/PhxEngine.h>

#include "EditorLayer.h"
#include "GuiLayer.h"

using namespace PhxEngine;
using namespace PhxEngine::Renderer;
using namespace PhxEngine::RHI;
using namespace PhxEngine::New;

namespace PhxEngine::Editor
{
	class PhxEngineApp : public Application
	{
	public:
		PhxEngineApp(
			RHI::IGraphicsDevice* graphicsDevice,
			ApplicationCommandLineArgs args)
			: Application(graphicsDevice, args, "Editor")
		{
			this->m_editorLayer = std::make_shared<EditorLayer>();
			this->PushBackLayer(this->m_editorLayer);

			this->m_editorGui = std::make_shared<GuiLayer>(
				this->GetGraphicsDevice(),
				this->GetWindow(),
				this->m_editorLayer);
			this->PushBackLayer(this->m_editorGui);
		}

	private:
		std::shared_ptr<EditorLayer> m_editorLayer;
		std::shared_ptr<GuiLayer> m_editorGui;
	};
}


std::unique_ptr<Application> PhxEngine::New::CreateApplication(ApplicationCommandLineArgs args, RHI::IGraphicsDevice* GraphicsDevice)
{
	return std::make_unique<Editor::PhxEngineApp>(GraphicsDevice, args);
}