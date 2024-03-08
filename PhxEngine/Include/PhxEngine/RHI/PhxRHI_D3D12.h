#pragma once

#include <PhxEngine/RHI/PhxRHI.h>

#ifdef _DEBUG
#include <dxgidebug.h>

#endif

#include "d3d12.h"
#include <d3dx12/d3dx12.h>
#include "d3d12sdklayers.h"
#include "d3d12shader.h"
#include <dxgi1_6.h>
#include <wrl.h>

namespace PhxEngine::RHI::D3D12
{
	class ID3D12GfxDevice : public RHI::IGfxDevice
	{
	public:
		virtual ~ID3D12GfxDevice() = default;

		virtual Microsoft::WRL::ComPtr<ID3D12Device> GetD3D12Device() = 0;
		virtual Microsoft::WRL::ComPtr<ID3D12Device2> GetD3D12Device2() = 0;
		virtual Microsoft::WRL::ComPtr<ID3D12Device5> GetD3D12Device5() = 0;
		virtual Microsoft::WRL::ComPtr<IDXGIFactory6> GetDxgiFactory() = 0;
		virtual Microsoft::WRL::ComPtr<IDXGIAdapter> GetDxgiAdapter() = 0;
	};
}