#include <PhxEngine/EntryPoint.h>
#include <PhxEngine/Application.h>

#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Renderer/RendererSubSystem.h>
#include <PhxEngine/World/World.h>
#include <PhxEngine/World/Entity.h>
#include <PhxEngine/Resource/ResourceManager.h>
#include <PhxEngine/Resource/Mesh.h>

class SandboxApp : public PhxEngine::IApplication
{
public:
	void Startup() override;
	void Shutdown() override;

	void PreRender() override {};
	void FixedUpdate() override {};
	void Update(PhxEngine::TimeStep const deltaTime) override {};
	void Render() override {};

private:
	PhxEngine::DrawableInstanceHandle m_sphereDrawable;
	PhxEngine::World m_world;
};

PHX_CREATE_APPLICATION(SandboxApp);

using namespace PhxEngine;

void SandboxApp::Startup()
{
	ResourceManager::RegisterPath("..\\resources");

	Entity cubeEntity = this->m_world.CreateEntity("Rotating Cube");
	auto& meshComponent = cubeEntity.AddComponent<MeshComponent>();
	meshComponent.Mesh = MeshResourceRegistry::Ptr->Retrieve(StringHash("CubeMesh"));
}

void SandboxApp::Shutdown()
{
}
