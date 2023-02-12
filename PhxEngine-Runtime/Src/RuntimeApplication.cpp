#include <PhxEngine/PhxEngine.h>
#include <PhxEngine/Engine/EntryPoint.h>


class RuntimeApplication : public PhxEngine::EngineApp
{
public:
	RuntimeApplication(PhxEngine::ApplicationSpecification const& spec)
		: EngineApp(spec)
	{};

	void OnInit() override
	{
		// TODO: Push Layer
		// this->PushLayer(EditorLayer);
	}
};

PhxEngine::EngineApp* PhxEngine::CreateApplication(int argc, char** argv)
{
	// Set up application
	ApplicationSpecification spec;
	spec.Name = "PhxEngine Editor";
	spec.FullScreen = false;
	spec.EnableImGui = true;
	spec.VSync = false;
	spec.AllowWindowResize = false;

	return new RuntimeApplication(spec);
}