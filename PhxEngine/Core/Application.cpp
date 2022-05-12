#include "phxpch.h"

#include "Application.h"

using namespace PhxEngine::Core;

void Application::Initialize(PhxEngine::RHI::IGraphicsDevice* graphicsDevice)
{
	this->m_graphicsDevice = graphicsDevice;

	// TODO: Initalize Sub Systems
}

void Application::Finalize()
{

}

void Application::RunFrame()
{
	// TODO: Initialization?
	TimeStep deltaTime = this->m_stopWatch.Elapsed();
	this->m_stopWatch.Begin();

	// Variable-timed update:
	this->Update(deltaTime);

	this->Render();

	this->Compose();
}

void Application::Update(TimeStep deltaTime)
{

}

void Application::Render()
{

}

void Application::Compose()
{
}