#pragma once

#include <RHI/RHIResources.h>
#include <Core/RefCountPtr.h>
#include <D3D12MemAlloc.h>
#include <memory>

namespace PhxEngine::RHI::D3D12
{
	class D3D12Device;
	class D3D12GpuMemoryAllocator final
	{
	public:
		D3D12GpuMemoryAllocator(std::shared_ptr<D3D12Device> device);
		~D3D12GpuMemoryAllocator() = default;
		
		GpuMemoryUsage GetMemoryUsage() const;

		Core::RefCountPtr<D3D12MA::Allocator> GetNativeAllocator() const { return this->m_d3d12MemAllocator; }

	private:
		Core::RefCountPtr<D3D12MA::Allocator> m_d3d12MemAllocator;
		
	};
}

