#pragma once

#include <RHI/RHIResources.h>
#include <Core/RefCountPtr.h>
#include <D3D12MemAlloc.h>
#include <memory>
#include "d3dx12.h"

namespace PhxEngine::RHI::D3D12
{
	class D3D12Device;
	class D3D12GpuMemoryAllocator final
	{
	public:
		D3D12GpuMemoryAllocator(std::shared_ptr<D3D12Device> device);
		~D3D12GpuMemoryAllocator() = default;
		
	public:
		void AllocateResource(
			D3D12MA::ALLOCATION_DESC const& allocationDesc,
			CD3DX12_RESOURCE_DESC const& resourceDesc,
			D3D12_RESOURCE_STATES initialState,
			D3D12MA::Allocation** outAllocation,
			Core::RefCountPtr<ID3D12Resource> d3d12Resource);

		void AllocateAliasingResource(
			D3D12MA::Allocation* aliasedResourceAllocation,
			CD3DX12_RESOURCE_DESC const& resourceDesc,
			D3D12_RESOURCE_STATES initialState, 
			Core::RefCountPtr<ID3D12Resource> d3d12Resource);

		void AllocateAliasedResource(
			D3D12MA::ALLOCATION_DESC const& allocationDesc,
			D3D12_RESOURCE_ALLOCATION_INFO const& finalAllocInfo,
			D3D12MA::Allocation** outAllocation);

	public:
		GpuMemoryUsage GetMemoryUsage() const;
		Core::RefCountPtr<D3D12MA::Allocator> GetNativeAllocator() const { return this->m_d3d12MemAllocator; }

	private:
		Core::RefCountPtr<D3D12MA::Allocator> m_d3d12MemAllocator;
		
	};
}

