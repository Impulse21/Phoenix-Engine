#include <PhxEngine/EntryPoint.h>
#include <PhxEngine/Application.h>


class SandboxApp : public PhxEngine::IApplication
{
public:
	void Startup() override {};
	void Shutdown() override {};
};

PHX_CREATE_APPLICATION(SandboxApp);