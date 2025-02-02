#pragma once

#include "D3D12Base.h"

namespace phx::rhi::d3d12
{
	class CopyCtxManager
	{
	public:
		struct Ctx
		{
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> Allocator = nullptr;
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList = nullptr;
			Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
			size_t FenceValue = 0;
			Microsoft::WRL::ComPtr<ID3D12Resource> UploadBuffer;
			Microsoft::WRL::ComPtr<D3D12MA::Allocation> UploadAllocation;
			size_t UploadBufferSize;
			void* MappedData = nullptr;
			inline bool IsValid() const { return CommandList != nullptr; }
			inline bool IsInValid() const { return CommandList == nullptr; }
			inline bool IsCompleted() const { return Fence->GetCompletedValue() >= FenceValue; }
		};

		void Initialize();
		void Finalize();

		Ctx Begin(size_t stagingSize);
		void Submit(Ctx uploadCtx);

	private:
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_copyQueue;
		std::vector<Ctx> m_freeList;
		std::mutex m_mutex;
	};
}