#pragma once

#include "D3D12Common.h"
#include <Core/RefCountPtr.h>
#include <RHI/RHIResources.h>

namespace PhxEngine::RHI::D3D12
{
	class D3D12CommandQueue;
	struct D3D12CommandList : NonCopyable, NonMoveable
	{
		Core::RefCountPtr<ID3D12GraphicsCommandList> NativeCmdList;
		Core::RefCountPtr<ID3D12GraphicsCommandList6> NativeCmdList6;
		D3D12CommandQueue* Queue; 
		ID3D12CommandAllocator* Allocator;

		D3D12CommandList(
			Core::RefCountPtr<ID3D12GraphicsCommandList> cmdList,
			D3D12CommandQueue* queue)
			: NativeCmdList(cmdList)
			, Queue(queue)
		{
			ThrowIfFailed(
				NativeCmdList->QueryInterface(IID_PPV_ARGS(&this->NativeCmdList6)));
		}

		void Reset(ID3D12CommandAllocator* allocator);

	};
}

