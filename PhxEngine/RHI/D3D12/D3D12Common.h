#pragma once


#include <Core/RefCountPtr.h>

#include <array>
#define NOMINMAX
#include <assert.h>

#include "d3d12.h"
#include "d3dx12.h"
#include <dxgi1_6.h>
#ifdef _DEBUG
#include <dxgidebug.h>

#endif

namespace PhxEngine::RHI::D3D12
{
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw std::exception();
        }
    }
}