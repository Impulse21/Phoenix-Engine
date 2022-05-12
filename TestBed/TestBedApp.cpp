
#include "PhxEngine.h"
#include "Core/EntryPoint.h"

using namespace PhxEngine::Core;

class TestApp : public Application
{
public:
};

PhxEngine::Core::Application* PhxEngine::Core::CreateApplication(CommandLineArgs args)
{
    return new TestApp();
}
