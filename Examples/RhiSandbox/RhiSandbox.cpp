#include <PhxEngine/PhxEngine.h>

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Core/EventDispatcher.h>
#include <PhxEngine/Renderer/RenderGraph/RenderGraph.h>

using namespace PhxEngine;

namespace TryOut
{
	enum class ReferenceType : unsigned int
	{
		Invalid,
		RenderTarget,
		PassResult,
		ExternalSRV

	};

	struct Reference
	{
		enum : uint8_t
		{
			AllSubresources = 0x7f
		};

		union
		{
			struct
			{
				ReferenceType Type : 2;
				unsigned int Depth : 1;
				unsigned int EntireTexture : 1;		 // Used for PassResult types to specify AllSubresources (since SubResource is used for the output index)
				unsigned int MipSlice : 1;
				unsigned int SubResource : 7;
				unsigned int MipLevel : 4;
				unsigned int Index : 16;
			};
			unsigned int Data;
		};

		Reference() {}

		constexpr Reference(const ReferenceType type, const unsigned int index, const unsigned int sub_resource, const unsigned int depth = 0, const unsigned int entire_texture = 0, const unsigned int mip_slice = 0, const unsigned int mip_level = 0) :
			Type(type),
			Depth(depth),
			EntireTexture(entire_texture),
			MipSlice(mip_slice),
			SubResource(sub_resource),
			MipLevel(mip_level),
			Index(index)
		{
			ASSERT(sub_resource < 128);
			ASSERT(index < 65536);
			ASSERT(mip_slice == 1 || mip_level == 0);
			ASSERT(mip_level < 16);
		}

		constexpr Reference SubResourceRef(const unsigned int sub_resource) const { return Reference(Type, Index, sub_resource, Depth); }
		constexpr Reference MipSliceRef(const unsigned int mip_level) const { return Reference(Type, Index, 0, Depth, 0, 1, mip_level); }

		static constexpr Reference Null()
		{
			return Reference(ReferenceType::Invalid, 0, 0);
		}

		explicit operator bool() const { return Data != 0; }
	};

	struct RenderTarget
	{
		int Index;

		RenderTarget() {}
		explicit RenderTarget(const int index) : Index(index) {}

		constexpr operator Reference() const { return Reference(ReferenceType::RenderTarget, Index, 0); }
		constexpr Reference SubResource(const unsigned int sub_resource) const { return Reference(ReferenceType::RenderTarget, Index, sub_resource); }
		constexpr Reference MipSlice(const unsigned int mip_level) const { return Reference(ReferenceType::RenderTarget, Index, 0, 0, 0, 1, mip_level); }
		constexpr Reference AllSubResources() const { return SubResource(Reference::AllSubresources); }
	};

	struct PassResult
	{
		int Index;

		PassResult() {}
		explicit PassResult(const int index) : Index(index) {}

		constexpr Reference Colour(const int index, const bool entire_texture = false) const { return Reference(ReferenceType::PassResult, Index, index, 0, entire_texture); }
		constexpr Reference Depth(const bool entire_texture = false) const { return Reference(ReferenceType::PassResult, Index, 0, 1, entire_texture); }
	};
}
namespace
{
	std::unique_ptr<Core::IWindow> window;
	RHI::SwapChainRef swapChain = nullptr;
	void Initialize()
	{
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