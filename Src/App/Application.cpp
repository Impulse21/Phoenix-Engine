#include <PhxEngine/App/Application.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/core/Asserts.h>
#include <PhxEngine/Core/FileSystem.h>
#include <PhxEngine/Core/TimeStep.h>
#include <PhxEngine/Core/Layer.h>

#include "GLFW/glfw3.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

using namespace PhxEngine;

bool ApplicationBase::sGlwfIsInitialzed = false;

namespace
{
	static bool sGlwfIsInitialzed = false;
	static std::atomic_uint sNumHeapAlloc = 0;
	static std::atomic_ullong sSizeOfHeapAlloc = 0;
}

New::Application* New::Application::sSingleton = nullptr;

void ApplicationBase::Initialize(EngineEnvironment* engineEnv)
{
	this->m_env = engineEnv;
	this->CreateGltfWindow();

	RHI::SwapChainDesc desc = {};
	desc.Format = RHI::EFormat::RGBA16_FLOAT;
	desc.Width = WindowWidth;
	desc.Height = WindowHeight;
	desc.WindowHandle = (void*)glfwGetWin32Window(this->m_window);

	this->GetGraphicsDevice()->CreateSwapChain(desc);
	this->m_commandList = this->GetGraphicsDevice()->CreateCommandList();

	this->m_baseFileSystem = std::make_shared<Core::NativeFileSystem>();
}

void PhxEngine::ApplicationBase::RunFrame()
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
			this->Render();
			this->RenderScene();
			this->RenderUI();
			this->GetGraphicsDevice()->Present();
		}

		this->UpdateAvarageFrameTime(elapsedTime);
		this->m_frameStatistics.PreviousFrameTimestamp = currFrameTime;
	}
}

void PhxEngine::ApplicationBase::Shutdown()
{
	this->GetGraphicsDevice()->WaitForIdle();
}

void PhxEngine::ApplicationBase::CreateGltfWindow()
{
	LOG_CORE_INFO("Creating window %s (%u, %u)", "PhxEngine", WindowWidth, WindowHeight);

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


PhxEngine::New::Application::Application(
	RHI::IGraphicsDevice* graphicsDevice,
	ApplicationCommandLineArgs args,
	std::string const& name)
	: m_name(name)
	, m_graphicsDevice(graphicsDevice)
	, m_commandLineArgs(args)
{
	PHX_ASSERT(!sSingleton);
	sSingleton = this;

	// Create GLTF window
	this->CreateGltfWindow(name, { 1280, 720 });

	RHI::SwapChainDesc swapchainDesc = {};
	swapchainDesc.Width = 1280;
	swapchainDesc.Height = 720;
	swapchainDesc.Format = RHI::EFormat::RGBA8_UNORM;
	// HDR - Looks white washed, not sure why?
	// swapchainDesc.Format = RHI::EFormat::RGBA16_FLOAT;
	swapchainDesc.WindowHandle = (void*)glfwGetWin32Window(this->m_window);

	this->m_graphicsDevice->CreateSwapChain(swapchainDesc);

	// Initialize any sub-systems
	// this->m_fontRenderer = std::make_unique<Renderer::FontRenderer>(this->m_graphicsDevice);
}

PhxEngine::New::Application::~Application()
{
	Application::sSingleton = nullptr;
}

void PhxEngine::New::Application::Run()
{
	while (!glfwWindowShouldClose(this->m_window))
	{
		glfwPollEvents();

		this->UpdateWindowSize();

		float time = (float)glfwGetTime();

		Core::TimeStep timestep = time - this->m_lastFrameTime;
		this->m_lastFrameTime = time;

		// DO Fix Frame Updae
		if (this->m_isWindowVisible)
		{
			this->Update(timestep);
			this->Render();
			this->GetGraphicsDevice()->Present();
		}

		sNumHeapAlloc.store(0);
		sSizeOfHeapAlloc.store(0);
	}

	// Clean up
	for (auto& layer : this->m_layerStack)
	{
		layer->OnDetach();

	}

	this->m_layerStack.clear();
	this->GetGraphicsDevice()->WaitForIdle();
}

void PhxEngine::New::Application::PushBackLayer(std::shared_ptr<Core::Layer> layer)
{
	this->m_layerStack.push_back(layer);
	layer->OnAttach();
}

void PhxEngine::New::Application::Update(Core::TimeStep const& elapsedTime)
{
	for (auto& layer : this->m_layerStack)
	{
		layer->OnUpdate(elapsedTime);
	}
}

void PhxEngine::New::Application::Render()
{
	RHI::TextureHandle currentBuffer = this->GetGraphicsDevice()->GetBackBuffer();
	for (auto& layer : this->m_layerStack)
	{
		layer->OnRender(currentBuffer);
	}
}

void PhxEngine::New::Application::UpdateWindowSize()
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
}

void PhxEngine::New::Application::CreateGltfWindow(std::string const& name, WindowDesc const& desc)
{
	LOG_CORE_INFO("Creating window %s (%u, %u)", name.c_str(), desc.Width, desc.Height);

	if (!sGlwfIsInitialzed)
	{
		glfwInit();
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	// TODO Check Flags
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	this->m_window =
		glfwCreateWindow(
			desc.Width,
			desc.Height,
			name.c_str(),
			nullptr,
			nullptr);

	this->m_windowDesc = desc;
	// glfwSetWindowUserPointer(this->m_window, &this->m_windowState);

	/*
	// Handle Close Event
	glfwSetWindowCloseCallback(this->m_window, [](GLFWwindow* window) {
		WindowState& state = *(WindowState*)glfwGetWindowUserPointer(window);
		state.IsClosing = true;
		});
	*/
}


// Heap alloc replacements are used to count heap allocations:
//	It is good practice to reduce the amount of heap allocations that happen during the frame,
//	so keep an eye on the info display of the engine while Application::InfoDisplayer::heap_allocation_counter is enabled

void* operator new(std::size_t size) {
	sNumHeapAlloc.fetch_add(1);
	sSizeOfHeapAlloc.fetch_add(size);
	void* p = malloc(size);
	if (!p) throw std::bad_alloc();
	return p;
}
void* operator new[](std::size_t size) {
	sNumHeapAlloc.fetch_add(1);
	sSizeOfHeapAlloc.fetch_add(size);
	void* p = malloc(size);
	if (!p) throw std::bad_alloc();
	return p;
}
void* operator new[](std::size_t size, const std::nothrow_t&) throw() {
	sNumHeapAlloc.fetch_add(1);
	sSizeOfHeapAlloc.fetch_add(size);
	return malloc(size);
}
void* operator new(std::size_t size, const std::nothrow_t&) throw() {
	sNumHeapAlloc.fetch_add(1);
	sSizeOfHeapAlloc.fetch_add(size);
	return malloc(size);
}
void operator delete(void* ptr) throw() { free(ptr); }
void operator delete (void* ptr, const std::nothrow_t&) throw() { free(ptr); }
void operator delete[](void* ptr) throw() { free(ptr); }
void operator delete[](void* ptr, const std::nothrow_t&) throw() { free(ptr); }