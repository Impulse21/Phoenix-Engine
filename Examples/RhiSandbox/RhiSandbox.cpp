#include <PhxEngine/PhxEngine.h>

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Core/EventDispatcher.h>
#include <PhxEngine/Renderer/RenderGraph/RenderGraph.h>

using namespace PhxEngine;

namespace
{
	std::unique_ptr<Core::IWindow> window;
	RHI::SwapChainRef swapChain = nullptr;
	void Initialize()
	{
		Renderer::RG::PassResult result(1);
		const Renderer::RG::Reference f = result.Colour(0, true);
		

		Core::Log::Initialize();
		RHI::Initialize(RHI::GraphicsAPI::DX12);

		window = Core::WindowFactory::CreateGlfwWindow({
		   .Width = 2000,
		   .Height = 1200,
		   .VSync = false,
		   .Fullscreen = false,
			});

		window->SetEventCallback([](Core::Event& e) { Core::EventDispatcher::DispatchEvent(e); });
		window->Initialize();

		swapChain = RHI::GetDynamic()->CreateSwapChain({
					.Width = window->GetWidth(),
					.Height = window->GetHeight(),
					.Fullscreen = false,
					.VSync = window->GetVSync() },
					window->GetNativeWindowHandle());

		Core::EventDispatcher::AddEventListener(Core::EventType::WindowResize, [&](Core::Event const& e) 
			{
				const Core::WindowResizeEvent& resizeEvent = static_cast<const Core::WindowResizeEvent&>(e);
				RHI::SwapChainDesc swapchainDesc = {
					.Width = resizeEvent.GetWidth(),
					.Height = resizeEvent.GetHeight(),
					.Fullscreen = false,
					.VSync = window->GetVSync() };
				RHI::GetDynamic()->ResizeSwapChain(swapChain, swapchainDesc);
			});
	}

	void Finalize()
	{
		swapChain.Reset();
		RHI::Finiailize();
		Core::Log::Finialize();
	}
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	Initialize();

	Core::LinearAllocator graphAllocator;
	graphAllocator.Initialize(PhxMB(1));
	while (!window->ShouldClose())
	{
		graphAllocator.Clear();
		window->OnTick();
		RHI::DynamicRHI* rhi = RHI::GetDynamic();

		Renderer::RenderGraphBuilder graph(&graphAllocator);
		
		// - Just testing API
		Renderer::RgResourceHandle albedoTex	= graph.CreateTexture();
		Renderer::RgResourceHandle normalTex	= graph.CreateTexture();
		Renderer::RgResourceHandle specularTex	= graph.CreateTexture();
		Renderer::RgResourceHandle emissiveTex	= graph.CreateTexture();

		const size_t size = sizeof(Renderer::RgResourceHandle);
		const size_t size1 = sizeof(uint64_t);
		graph.AddPass(
			"FillGBuffer",
			{},
			{ albedoTex, normalTex, specularTex, emissiveTex },
			Renderer::RgPassFlags::Raster,
			[](Renderer::RgRegistry const& registry, RHI::CommandListRef commandList)
			{
				LOG_INFO("FillGBuffer");
			});

		Renderer::RgResourceHandle lightingBuffer = graph.CreateTexture();
		graph.AddPass(
			"LightingPasss",
			{ albedoTex, normalTex, specularTex, emissiveTex },
			{ lightingBuffer },
			Renderer::RgPassFlags::Raster,
			[](Renderer::RgRegistry const& registry, RHI::CommandListRef commandList)
			{
				LOG_INFO("LightingPasss");
			});

		Renderer::RgResourceHandle backBuffer = graph.RegisterExternalTexture(nullptr);
		graph.AddPass(
			"TonMapping",
			{ lightingBuffer },
			{ backBuffer },
			Renderer::RgPassFlags::Raster,
			[](Renderer::RgRegistry const& registry, RHI::CommandListRef commandList)
			{
				LOG_INFO("TonMapping");
			});

		graph.Execute();
		rhi->Present(swapChain);
		break;
	}

	graphAllocator.Finalize();
	::Finalize();
	// Create SwapChain
}