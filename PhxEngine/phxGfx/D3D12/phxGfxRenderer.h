#pragma once

#include "phxGfxCore.h"
#include <atomic>
namespace phx::gfx
{
	class D3D12CommandList final : public CommandList
	{
	public:
		D3D12CommandList() = default;
		~D3D12CommandList() = default;

	public:
		RenderPassRenderer* BeginRenderPass() override;
	};

	class D3D12RenderPassRenderer final: public RenderPassRenderer
	{
	public:
		D3D12RenderPassRenderer() = default;
		~D3D12RenderPassRenderer() = default;
	};

	class D3D12Renderer final : public Renderer
	{
	public:
		inline static D3D12Renderer* Impl() { return Singleton; }

	public:
		D3D12Renderer() 
		{
			Singleton = this;
		}
		~D3D12Renderer()
		{
			Singleton = nullptr;
		};

	public:
		void SubmitCommands() override;
		CommandList* BeginCommandRecording(CommandQueueType type = CommandQueueType::Graphics) override;

	private:
		inline static D3D12Renderer* Singleton = nullptr;

		std::atomic_uint32_t m_activeCmdCount;
		std::array<D3D12CommandList, 32> m_commandListPool;
	};


}