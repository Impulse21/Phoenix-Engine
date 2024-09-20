#include "pch.h"

#include "phxGfxGpuMemoryManager.h"
#include "phxGfxDevice.h"

using namespace phx;
using namespace phx::gfx;
using namespace dx;

phx::gfx::D3D12GpuMemoryManager::D3D12GpuMemoryManager(D3D12Device* device)
{
	Singleton = this;

	D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
	allocatorDesc.pDevice = device->GetD3D12Device();
	allocatorDesc.pAdapter = device->GetDxgiAdapter();
	//allocatorDesc.PreferredBlockSize = 256 * 1024 * 1024;
	//allocatorDesc.Flags |= D3D12MA::ALLOCATOR_FLAG_ALWAYS_COMMITTED;
	allocatorDesc.Flags = (D3D12MA::ALLOCATOR_FLAGS)(D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED);

	ThrowIfFailed(
		D3D12MA::CreateAllocator(&allocatorDesc, &this->m_d3d12MemAllocator));
}

phx::gfx::D3D12GpuMemoryManager::~D3D12GpuMemoryManager()
{
	Singleton = nullptr;
}
