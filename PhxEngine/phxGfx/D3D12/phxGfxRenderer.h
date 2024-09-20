#pragma once

#include "phxGfxCore.h"
#include <atomic>
#include "pch.h"

namespace phx::gfx
{
	struct D3D12Semaphore
	{
		Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
		uint64_t fenceValue = 0;
	};

	class D3D12CommandList;
	class D3D12RenderPassRenderer final : public RenderPassRenderer
	{
	public:
		D3D12RenderPassRenderer() = default;
		~D3D12RenderPassRenderer() = default;

	public:
		void Initialize(ID3D12GraphicsCommandList* commandList, ID3D12GraphicsCommandList6* commandList6);
		void Begin(D3D12_CPU_DESCRIPTOR_HANDLE rtvView);

	private:
		ID3D12GraphicsCommandList* m_commandList;
		ID3D12GraphicsCommandList6* m_commadList6;

	};

	class D3D12CommandList final : public CommandList
	{
		friend class D3D12Renderer;
	public:
		D3D12CommandList() = default;
		~D3D12CommandList() = default;

	public:
		RenderPassRenderer* BeginRenderPass() override;
		void EndRenderPass(RenderPassRenderer* pass) override;


	public:
		ID3D12GraphicsCommandList* GetD3D12CmdList() { return this->m_commandList.Get(); }
		ID3D12GraphicsCommandList6* GetD3D12CmdList6() { return this->m_commandList6.Get(); }

	private:
		uint32_t m_id = 0;
		CommandQueueType m_queueType = CommandQueueType::Graphics;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> m_commandList6;
		ID3D12CommandAllocator* m_allocator;
		std::atomic_bool m_isWaitedOn = false;
		std::vector<D3D12Semaphore> m_waits;

		D3D12RenderPassRenderer m_activeRenderPass;
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