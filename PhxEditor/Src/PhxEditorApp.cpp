#include <PhxEngine/Core/EntryPoint.h>
#include <PhxEngine/Core/Application.h>

#include <PhxEngine/Core/Math.h>
#include <PhxEngine/World/World.h>
#include <PhxEngine/World/Entity.h>
#include <PhxEngine/Resource/ResourceManager.h>
#include <PhxEngine/Resource/Mesh.h>

#include <imgui.h>

class DemoLayer : public PhxEngine::Layer
{
public:
	DemoLayer()
		: Layer("DemoLayer")
	{}


	void OnImGuiRender() override 
	{
		ImGui::ShowDemoWindow();
	}
};

class PhxEditor : public PhxEngine::Application
{
public:
	PhxEditor()
	{
		this->PushOverlay<DemoLayer>();
	}
};

PhxEngine::Application* PhxEngine::CreateApplication(ApplicationCommandLineArgs args)
{
	return new PhxEditor();
}

