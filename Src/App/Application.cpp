#include <PhxEngine/App/Application.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/core/Asserts.h>

#include "GLFW/glfw3.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

using namespace PhxEngine;

bool ApplicationBase::sGlwfIsInitialzed = false;

void ApplicationBase::Initialize(RHI::IGraphicsDevice* graphicsDevice)
{
	PHX_ASSERT(graphicsDevice);
	this->m_graphicsDevice = graphicsDevice;
	this->CreateGltfWindow();

	RHI::SwapChainDesc desc = {};
	desc.Format = RHI::EFormat::RGBA8_UNORM;
	desc.Width = WindowWidth;
	desc.Height = WindowHeight;
	desc.WindowHandle = (void*)glfwGetWin32Window(this->m_window);

	this->m_graphicsDevice->CreateSwapChain(desc);
	this->m_commandList = this->m_graphicsDevice->CreateCommandList();
}

void PhxEngine::ApplicationBase::Run()
{
	this->m_frameStatistics.PreviousFrameTimestamp = glfwGetTime();

	this->LoadContent();
	while (!glfwWindowShouldClose(this->m_window))
	{
		glfwPollEvents();

		this->UpdateWindowSize();

		double currFrameTime = glfwGetTime();
		double elapsedTime = currFrameTime - this->m_frameStatistics.PreviousFrameTimestamp;

		// Update Inputs
		if (this->m_isWindowVisible)
		{
			this->Update(elapsedTime);
			this->RenderScene();
			this->RenderUI();
			this->m_graphicsDevice->Present();
		}

		this->UpdateAvarageFrameTime(elapsedTime);
		this->m_frameStatistics.PreviousFrameTimestamp = currFrameTime;
	}
}

void PhxEngine::ApplicationBase::Shutdown()
{
	this->m_graphicsDevice->WaitForIdle();
}

void PhxEngine::ApplicationBase::CreateGltfWindow()
{
	LOG_CORE_INFO("Creating window {0} ({1}, {2})", "PhxEngine", WindowWidth, WindowHeight);

	if (!sGlwfIsInitialzed)
	{
		glfwInit();
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// TODO Check Flags
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	this->m_window =
		glfwCreateWindow(
			WindowWidth,
			WindowHeight,
			"PhxEngine",
			nullptr,
			nullptr);

	// glfwSetWindowUserPointer(this->m_window, &this->m_windowState);

	/*
	// Handle Close Event
	glfwSetWindowCloseCallback(this->m_window, [](GLFWwindow* window) {
		WindowState& state = *(WindowState*)glfwGetWindowUserPointer(window);
		state.IsClosing = true;
		});
	*/
}


void ApplicationBase::UpdateAvarageFrameTime(double elapsedTime)
{
	this->m_frameStatistics.FrameTimeSum += elapsedTime;
	this->m_frameStatistics.NumberOfAccumulatedFrames += 1;

	if (this->m_frameStatistics.FrameTimeSum > this->m_frameStatistics.AverageTimeUpdateInterval && this->m_frameStatistics.NumberOfAccumulatedFrames > 0)
	{
		this->m_frameStatistics.AverageFrameTime = this->m_frameStatistics.FrameTimeSum / double(this->m_frameStatistics.NumberOfAccumulatedFrames);
		this->m_frameStatistics.NumberOfAccumulatedFrames = 0;
		this->m_frameStatistics.FrameTimeSum = 0.0;
	}
}

void ApplicationBase::UpdateWindowSize()
{
	int width;
	int height;
	glfwGetWindowSize(this->m_window, &width, &height);

	if (width == 0 || height == 0)
	{
		// window is minimized
		this->m_isWindowVisible = false;
		return;
	}

	this->m_isWindowVisible = true;

	// TODO: Update back buffers if resize
}