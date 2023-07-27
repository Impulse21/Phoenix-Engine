
#include <PhxEngine/PhxEngine.h>
#include <PhxEngine/Engine/EntryPoint.h>

#include "EditorLayer.h"
#include "SceneRenderLayer.h"

class EditorApplication : public PhxEngine::EngineApp
{
public:
	EditorApplication(PhxEngine::ApplicationSpecification const& spec)
		: EngineApp(spec)
	{};

	void OnInit() override
	{
		auto sceneRenderLayer = std::make_shared<SceneRenderLayer>();
		this->PushLayer(std::make_shared<EditorLayer>(sceneRenderLayer));
		this->PushLayer(sceneRenderLayer);
	}
};

PhxEngine::EngineApp* PhxEngine::CreateApplication(int argc, char** argv)
{
	ApplicationSpecification spec;

	spec.Name = "PhxEngine Editor";
	spec.FullScreen = false;
	spec.EnableImGui = true;
	spec.VSync = false;
	spec.AllowWindowResize = false;

	return new EditorApplication(spec);
}