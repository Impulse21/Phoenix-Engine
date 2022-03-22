
#include <memory>

#include <DirectXMath.h>
#include <PhxEngine/App/EntryPoint.h>

#include <PhxEngine/PhxEngine.h>

#include "EditorLayer.h"

using namespace PhxEngine;
using namespace PhxEngine::Renderer;
using namespace PhxEngine::RHI;
using namespace PhxEngine::New;

class PhxEngineApp : public Application
{
public:
	PhxEngineApp(
		RHI::IGraphicsDevice* graphicsDevice,
		ApplicationCommandLineArgs args)
		: Application(graphicsDevice, args, "Editor")
	{
		this->m_editorLayer = std::make_unique<EditorLayer>();
		this->PushLayer(this->m_editorLayer.get());
	}

private:
	std::unique_ptr<EditorLayer> m_editorLayer;
};


std::unique_ptr<Application> PhxEngine::New::CreateApplication(ApplicationCommandLineArgs args, RHI::IGraphicsDevice* GraphicsDevice)
{
	return std::make_unique<PhxEngineApp>(GraphicsDevice, args);
}