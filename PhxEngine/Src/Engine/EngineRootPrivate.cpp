#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"

#include "EngineRootPrivate.h"
#include <PhxEngine/Core/Window.h>


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

	this->m_rhi = RHI::PlatformCreateRHI();
	this->m_rhi->Initialize();

	WindowSpecification windowSpec = {};
	windowSpec.Width = this->m_params.WindowWidth;
	windowSpec.Height = this->m_params.WindowHeight;
	windowSpec.Title = this->m_params.Name;
	windowSpec.VSync = this->m_params.VSync;

	this->m_window = WindowFactory::CreateGlfwWindow(this, windowSpec);
	this->m_window->Initialize();
	this->m_window->SetResizeable(false);
	this->m_window->SetVSync(this->m_params.VSync);
}

void PhxEngine::PhxEngineRoot::Finalizing()
{
	LOG_CORE_INFO("PhxEngine is Finalizing.");
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

		this->m_profile.UpdateAverageFrameTime(deltaTime.GetMilliseconds());
		this->m_frameCount++;
	}

	// this->m_rhi->WaitForIdle();
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

void PhxEngine::PhxEngineRoot::Update(TimeStep const& deltaTime)
{
	for (EngineRenderPass* renderPass : this->m_renderPasses)
	{
		renderPass->Update(deltaTime);
	}
}

void PhxEngine::PhxEngineRoot::Render()
{
	RHI::IRHIFrameRenderContext* frameRenderContext = this->GetRHI()->BeginFrameRenderContext(this->m_viewport);

	for (EngineRenderPass* renderPass : this->m_renderPasses)
	{
		renderPass->Render(frameRenderContext);
	}
}
