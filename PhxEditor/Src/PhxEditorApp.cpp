#include <PhxEngine/Core/EntryPoint.h>
#include <PhxEngine/Core/Application.h>

#include <PhxEngine/Core/Math.h>
#include <PhxEngine/World/World.h>
#include <PhxEngine/World/Entity.h>
#include <PhxEngine/Resource/ResourceManager.h>
#include <PhxEngine/Resource/Mesh.h>

class PhxEditor : public PhxEngine::Application
{
public:
};

PhxEngine::Application* PhxEngine::CreateApplication(ApplicationCommandLineArgs args)
{
	return new PhxEditor();
}

