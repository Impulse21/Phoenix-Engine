#pragma once

#ifdef USING_DIRECTX_HEADERS
#include "directx/dxgiformat.h"
#include "directx/d3d12.h"
#include "directx/d3dx12.h"
#include "dxguids/dxguids.h"
#else
#include <d3d12.h>
#include "d3dx12\d3dx12.h"
#endif

#include <dxgi1_6.h>

namespace phx::rhi::d3d12
{
	// Helper class for COM exceptions
	class com_exception : public std::exception
	{
	public:
		com_exception(HRESULT hr) noexcept : result(hr) {}

		const char* what() const noexcept override
		{
			static char s_str[64] = {};
			sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
			return s_str;
		}

	private:
		HRESULT result;
	};

	// Helper utility converts D3D API failures into exceptions.
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			// Set a breakpoint on this line to catch DirectX API errors
			throw com_exception(hr);
		}
	}
}