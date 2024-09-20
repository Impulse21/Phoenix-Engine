#pragma once

#include "phxGfxCore.h"

#include "d3d12ma/D3D12MemAlloc.h"

namespace phx::gfx
{
	class D3D12Device;
	class D3D12GpuMemoryManager final : public GpuMemoryManager
	{
	public:
		inline static D3D12GpuMemoryManager* Impl() { return Singleton; }

	public:
		D3D12GpuMemoryManager(D3D12Device* device);
		~D3D12GpuMemoryManager();

	private:
		inline static D3D12GpuMemoryManager* Singleton = nullptr;

		Microsoft::WRL::ComPtr<D3D12MA::Allocator> m_d3d12MemAllocator;
	};
}

