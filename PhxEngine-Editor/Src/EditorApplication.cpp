
#include <PhxEngine.h>
#include <App/EntryPoint.h>


class EditorApplication : public PhxEngine::Application
{
public:
	EditorApplication() = default;
};

PhxEngine::Application* PhxEngine::CreateApplication(int argc, char** argv)
{
	// Set up application
	return new EditorApplication();
}