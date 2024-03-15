#include <PhxEngine/EntryPoint.h>
#include <PhxEngine/Application.h>

#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Renderer/RendererSubSystem.h>


class SandboxApp : public PhxEngine::IApplication
{
public:
	void Startup() override;
	void Shutdown() override;

private:
	PhxEngine::DrawableHandle m_sphereDrawable;
};

PHX_CREATE_APPLICATION(SandboxApp);

void SandboxApp::Startup()
{
	// Load  mesh and render it.
	this->m_sphereDrawable = PhxEngine::RendererSubSystem::Ptr->DrawableCreate({
			.TransformMatrix = PhxEngine::cIdentityMatrix
		});
}

void SandboxApp::Shutdown()
{
	PhxEngine::RendererSubSystem::Ptr->DrawableDelete(this->m_sphereDrawable);
}
