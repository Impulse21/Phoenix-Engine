#include "phxpch.h"

#include "D3D12CopyCtxManager.h"

#include <phx/core/Math.h>

#include "D3D12Core.h"
#include "D3D12CommandQueue.h"

using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::d3d12;

void CopyCtxManager::Initialize()
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(
		g_d3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_copyQueue)));

	ThrowIfFailed(
		m_copyQueue->SetName(L"Copy Ctx Manager"));
}

void CopyCtxManager::Finalize()
{
}

CopyCtxManager::Ctx CopyCtxManager::Begin(size_t stagingSize)
{
	PHX_ASSERT(m_copyQueue, "Ensure Intialize has been called");
	Ctx retVal;
	{
		std::scoped_lock _(m_mutex);

		for (size_t i = 0; i < m_freeList.size(); i++)
		{
			Ctx& ctx = m_freeList[i];
			if (ctx.UploadBufferSize > stagingSize)
				continue;

			if (!ctx.IsCompleted())
				continue;

			retVal = ctx;
			std::swap(m_freeList[i], m_freeList.back());
			m_freeList.pop_back();
		}
	}

	if (retVal.IsInValid())
	{
		HRESULT hr = g_d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&retVal.Allocator));
		assert(SUCCEEDED(hr));

		hr = g_d3d12Device2->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_COPY,
			retVal.Allocator.Get(),
			nullptr,
			IID_PPV_ARGS(&retVal.CommandList));
		assert(SUCCEEDED(hr));

		retVal.CommandList->Close();

		hr = g_d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&retVal.Fence));
		assert(SUCCEEDED(hr));

		retVal.UploadBufferSize = math::GetNextPowerOfTwo(stagingSize);
		
		D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
		auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(
			retVal.UploadBufferSize,
			resourceFlags,
			D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

		ThrowIfFailed(
			g_d3d12MemAllocator->CreateResource(
				&allocationDesc,
				&resourceDesc,
				initialState,
				nullptr,
				&retVal.UploadAllocation,
				IID_PPV_ARGS(&retVal.UploadBuffer)));

		D3D12_RANGE readRange = {};
		ThrowIfFailed(
			retVal.UploadBuffer->Map(0, &readRange, &retVal.MappedData));
	}

	// begin command list in valid state:
	HRESULT hr = retVal.Allocator->Reset();
	PHX_ASSERT(SUCCEEDED(hr));
	hr = retVal.CommandList->Reset(retVal.Allocator.Get(), nullptr);
	PHX_ASSERT(SUCCEEDED(hr));

	return retVal;
}

void CopyCtxManager::Submit(Ctx ctx)
{
	HRESULT hr;

	{
		std::scoped_lock _(m_mutex);
		ctx.FenceValue++;
		m_freeList.push_back(ctx);
	}

	hr = ctx.CommandList->Close();
	PHX_ASSERT(SUCCEEDED(hr));

	ID3D12CommandList* commandlists[] = {
		ctx.CommandList.Get()
	};

	m_copyQueue->ExecuteCommandLists(1, commandlists);
	hr = m_copyQueue->Signal(ctx.Fence.Get(), ctx.FenceValue);
	assert(SUCCEEDED(hr));

	hr = g_commandQueue[CommandQueueType::Graphics].Queue->Wait(ctx.Fence.Get(), ctx.FenceValue);
	assert(SUCCEEDED(hr));

	hr = g_commandQueue[CommandQueueType::Compute].Queue->Wait(ctx.Fence.Get(), ctx.FenceValue);
	assert(SUCCEEDED(hr));

	hr = g_commandQueue[CommandQueueType::Copy].Queue->Wait(ctx.Fence.Get(), ctx.FenceValue);
	assert(SUCCEEDED(hr));
}
