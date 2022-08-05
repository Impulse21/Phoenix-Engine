#include <PhxEngine.h>
#include <App/EntryPoint.h>


class RuntimeApplication : public PhxEngine::Application
{
public:
	RuntimeApplication() = default;
};

PhxEngine::Application* PhxEngine::CreateApplication(int argc, char** argv)
{
	// Set up application
	return new RuntimeApplication();
}