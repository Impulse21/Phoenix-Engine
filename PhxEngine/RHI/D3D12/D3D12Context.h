#pragma once

#include <Core/RefCountPtr.h>

#define NOMINMAX
#include <assert.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#ifdef _DEBUG
#pragma comment(lib, "dxguid.lib")
#endif


#include "d3d12.h"
#include "d3dx12.h"
#include <dxgi1_6.h>
#ifdef _DEBUG
#include <dxgidebug.h>

#endif

#include <Core/Log.h>

#include "D3D12Adapter.h"

namespace PhxEngine::RHI::D3D12
{
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw std::exception();
        }
    }
	namespace D3D12Context
	{
		void Initialize(D3D12Adapter const& gpuAdapter);
		void Finalize();

		const D3D12Adapter& GetGpuAdapter();
		Core::RefCountPtr<IDXGIFactory6> GetDxgiFctory6();
		Core::RefCountPtr<ID3D12Device>  GetD3D12Device();
		Core::RefCountPtr<ID3D12Device2> GetD3D12Device2();
		Core::RefCountPtr<ID3D12Device5> GetD3D12Device5();

		bool IsUnderGraphicsDebugger = false;
	}
}

