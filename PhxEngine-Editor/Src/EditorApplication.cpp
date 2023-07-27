
#include <PhxEngine/PhxEngine.h>
#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Core/Window.h>

#ifdef _MSC_VER // Windows
#include <shellapi.h>
#endif 

using namespace PhxEngine;
using namespace PhxEngine::Core;

class EditorApplication : public IEngineApp
{
public:
	void Startup();
	void Shutdown() {};
	bool IsShuttingDown() { return false; }

	void OnTick();

private:
	std::unique_ptr<IWindow> m_editorWindow;
	RHI::SwapChain m_swapChain;
};


#ifdef _MSC_VER // Windows
#include <Windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else // Linux
int main(int argc, char** argv)
#endif
{
	// Engine Parameters?
	int argCount = 0;
#ifdef _MSC_VER
	// CommandLineArgs args(lpCmdLine);
#endif
	
	EngineCore::Initialize();
	{
		EditorApplication editor;
		EngineCore::RunApplication(editor);
	}
	EngineCore::Finalize();}

void EditorApplication::Startup()
{
	this->m_editorWindow = WindowFactory::CreateGlfwWindow({
		.Width = 2000,
		.Height = 1200,
		.VSync = false,
		.Fullscreen = false,
		});

	RHI::SwapChainDesc swapchainDesc = {
		.Width = this->m_editorWindow->GetWidth(),
		.Height = this->m_editorWindow->GetHeight(),
		.Fullscreen = false,
		.VSync = this->m_editorWindow->GetVSync(),
	};

	// Construct the swapchain
	// TODO: move to renderer
	RHI::Factory::CreateSwapChain(swapchainDesc, this->m_editorWindow->GetNativeWindow(), this->m_swapChain);
}

void EditorApplication::OnTick()
{
	// TODO: Move to renderer
	RHI::Present(this->m_swapChain);
}

