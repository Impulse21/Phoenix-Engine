#include <PhxEngine/PhxEngine.h>
#include <PhxEngine/App/EntryPoint.h>


class RuntimeApplication : public PhxEngine::LayeredApplication
{
public:
	RuntimeApplication(PhxEngine::ApplicationSpecification const& spec)
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
	spec.VSync = false;
	spec.AllowWindowResize = false;

	return new RuntimeApplication(spec);
}