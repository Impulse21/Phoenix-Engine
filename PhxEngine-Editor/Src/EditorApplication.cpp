
#include <PhxEngine.h>
#include <App/EntryPoint.h>

#include "EditorLayer.h"

class EditorApplication : public PhxEngine::LayeredApplication
{
public:
	EditorApplication(PhxEngine::ApplicationSpecification const& spec)
		: LayeredApplication(spec)
	{};

	void OnInit() override
	{
		// TODO: Push Layer
		// this->PushLayer(EditorLayer);
	}
};

PhxEngine::LayeredApplication* PhxEngine::CreateApplication(int argc, char** argv)
{
	// Set up application

	ApplicationSpecification spec;
	spec.Name = "PhxEngine Editor";
	spec.FullScreen = false;
	spec.EnableImGui = true;
	spec.VSync = true;
	spec.AllowWindowResize = false;

	return new EditorApplication(spec);
}