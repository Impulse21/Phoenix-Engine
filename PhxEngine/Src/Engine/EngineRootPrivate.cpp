#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"

#include "EngineRootPrivate.h"
#include <PhxEngine/Core/Window.h>
#include <PhxEngine/Engine/ApplicationEvents.h>


using namespace PhxEngine;
using namespace PhxEngine::Core;

std::unique_ptr<PhxEngine::IPhxEngineRoot> PhxEngine::CreateEngineRoot()
{
	return std::make_unique<PhxEngineRoot>();
}

PhxEngine::PhxEngineRoot::PhxEngineRoot()
	: m_frameCount(0)
	, m_profile({})
{
	assert(!PhxEngineRoot::sSingleton);
	PhxEngineRoot::sSingleton = this;
}

void PhxEngine::PhxEngineRoot::Initialize(EngineParam const& params)
{
	this->m_params = params;
	Core::Log::Initialize();

	LOG_CORE_INFO("Initializing Engine Root");

	this->m_gfxDevice = RHI::CreatePlatformGfxDevice();
	this->m_gfxDevice->Initialize();

	WindowSpecification windowSpec = {};
	windowSpec.Width = this->m_params.WindowWidth;
	windowSpec.Height = this->m_params.WindowHeight;
	windowSpec.Title = this->m_params.Name;
	windowSpec.VSync = this->m_params.VSync;

	this->m_window = WindowFactory::CreateGlfwWindow(this, windowSpec);
	this->m_window->Initialize();
	this->m_window->SetResizeable(false);
	this->m_window->SetVSync(this->m_params.VSync);
	this->m_window->SetEventCallback(
		[this](Event& e) {
			this->ProcessEvent(e);
		});

	this->GetGfxDevice()->CreateViewport(
		{
			.WindowHandle = static_cast<Platform::WindowHandle>(m_window->GetNativeWindowHandle()),
			.Width = this->m_window->GetWidth(),
			.Height = this->m_window->GetHeight(),
			.Format = RHI::RHIFormat::R10G10B10A2_UNORM,
		});

	this->m_canvasSize.x = windowSpec.Width;
	this->m_canvasSize.y = windowSpec.Height;
}

void PhxEngine::PhxEngineRoot::Finalizing()
{
	LOG_CORE_INFO("PhxEngine is Finalizing.");
	this->m_gfxDevice->Finalize();
}

void PhxEngine::PhxEngineRoot::Run()
{
	while (!this->m_window->ShouldClose())
	{
		TimeStep deltaTime = this->m_frameTimer.Elapsed();
		this->m_frameTimer.Begin();

		// polls events.
		this->m_window->OnUpdate();
		
		if (this->m_windowIsVisibile)
		{
			this->Update(deltaTime);
			this->Render();
		}

		this->m_profile.UpdateAverageFrameTime(deltaTime);
		this->m_frameCount++;
	}

	this->m_gfxDevice->WaitForIdle();
}

void PhxEngine::PhxEngineRoot::AddPassToBack(EngineRenderPass* pass)
{
	this->m_renderPasses.push_back(pass);
}

void PhxEngine::PhxEngineRoot::RemovePass(EngineRenderPass* pass)
{
	// Doesn't compress the vector, Not an issue right now as I don't have a use case for it yet.
	// Consider using a std::list.
	auto itr = std::find(this->m_renderPasses.begin(), this->m_renderPasses.end(), pass);
	if (itr != this->m_renderPasses.end())
	{
		this->m_renderPasses.erase(itr);
	}
}

void PhxEngine::PhxEngineRoot::SetInformativeWindowTitle(std::string_view appName, std::string_view extraInfo)
{
	std::stringstream ss;
	ss << appName;
	ss << "(D3D12)";

	double frameTime = this->m_profile.AverageFrameTime;
	if (frameTime > 0)
	{
		// ss << "=>" << std::setprecision(4) << frameTime << " CPU (ms) - " << std::setprecision(4) << (1.0f / frameTime) << " FPS";
		ss << "- " << std::setprecision(4) << (1.0f / frameTime) << " FPS";
	}

	if (!extraInfo.empty())
	{
		ss << extraInfo;
	}

	this->m_window->SetWindowTitle(ss.str());
}

void PhxEngine::PhxEngineRoot::Update(TimeStep const& deltaTime)
{
	for (EngineRenderPass* renderPass : this->m_renderPasses)
	{
		renderPass->Update(deltaTime);
	}
}

void PhxEngine::PhxEngineRoot::Render()
{
	this->m_gfxDevice->BeginFrame();

	for (EngineRenderPass* renderPass : this->m_renderPasses)
	{
		renderPass->Render();
	}

	this->m_gfxDevice->EndFrame();
}

void PhxEngine::PhxEngineRoot::ProcessEvent(Event& e)
{
	if (e.GetEventType() == WindowCloseEvent::GetStaticType())
	{
		e.IsHandled = true;
	}

	if (e.GetEventType() == WindowResizeEvent::GetStaticType())
	{
		WindowResizeEvent& resizeEvent = static_cast<WindowResizeEvent&>(e);
		if (resizeEvent.GetWidth() == 0 && resizeEvent.GetHeight() == 0)
		{
			this->m_windowIsVisibile = true;
		}
		else
		{
			this->m_windowIsVisibile = false;
			// Notify GFX to resize!
			
			// Trigger Resize event on RHI;
			for (auto& pass : this->m_renderPasses)
			{
				pass->OnWindowResize(resizeEvent);
			}
		}
	}
}

