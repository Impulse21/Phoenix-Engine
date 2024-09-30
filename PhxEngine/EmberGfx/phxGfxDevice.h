#pragma once

#include "D3D12/phxGfxPlatform.h"
#include "phxGfxCommandCtx.h"
#include "EmberGfx/phxHandle.h"

namespace phx::gfx
{
	class GfxDevice : NonCopyable
	{
	public:
		static void Initialize(SwapChainDesc const& swapChainDesc, void* windowHandle = nullptr)
		{
			PlatformGfxDevice::Initialize(swapChainDesc, windowHandle);
		}

		static void Finalize()
		{
			PlatformGfxDevice::Finalize();
		}

		static void WaitForIdle()
		{
			PlatformGfxDevice::WaitForIdle();
		}

		static void ResizeSwapChain(SwapChainDesc const& swapChainDesc)
		{
			PlatformGfxDevice::ResizeSwapChain(swapChainDesc);
		}

		static CommandCtx BeginGfxContext()
		{
			return PlatformGfxDevice::BeginGfxContext();
		}

		static CommandCtx BeginComputeContext()
		{
			return PlatformGfxDevice::BeginComputeContext();
		}

		static void SubmitFrame()
		{
			PlatformGfxDevice::SubmitFrame();
		}

		static TempBuffer AllocateTemp(size_t sizeInBytes, size_t alignment = 16)
		{
			return PlatformGfxDevice::AllocateTemp(sizeInBytes, alignment);
		}

	public:
		static GfxPipelineHandle CreateGfxPipeline(GfxPipelineDesc const& desc)
		{
			return PlatformGfxDevice::CreateGfxPipeline(desc);
		}

		static void DeleteResource(GfxPipelineHandle handle)
		{
			PlatformGfxDevice::DeleteResource(handle);
		}

		static TextureHandle CreateTexture(TextureDesc const& texture)
		{
			return PlatformGfxDevice::CreateTexture(texture);
		}

		static void DeleteResource(TextureHandle handle)
		{
			PlatformGfxDevice::DeleteResource(handle);
		}

		static BufferHandle CreateBuffer(BufferDesc const& desc)
		{
			return PlatformGfxDevice::CreateBuffer(desc);
		}

		static void DeleteResource(BufferHandle handle)
		{
			PlatformGfxDevice::DeleteResource(handle);
		}

		static InputLayoutHandle CreateInputLayout(Span<VertexAttributeDesc> desc)
		{
			return PlatformGfxDevice::CreateInputLayout(desc);
		}

		static void DeleteResource(InputLayoutHandle handle)
		{
			PlatformGfxDevice::DeleteResource(handle);
		}

	private:
	};


	template<typename HT>
	struct HandleOwner : NonCopyable
	{
		Handle<HT> m_handle;

		operator Handle<HT>() { return this->m_handle; }
		operator Handle<HT>() const { return this->m_handle; }

		HandleOwner() = default;
		HandleOwner(Handle<HT> handle) : m_handle(handle) {};
		~HandleOwner()
		{
			this->Reset();
		}

		void Reset(Handle<HT> handle = {})
		{
			if (m_handle.IsValid())
				GfxDevice::DeleteResource(m_handle);

			if (handle.IsValid())
				this->m_handle = handle;
		}

		Handle<HT> Release()
		{
			Handle<HT> retVal = this->m_handle;
			this->m_handle = {};
			return retVal;
		}
	};

}