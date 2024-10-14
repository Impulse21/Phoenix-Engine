#include "pch.h"
#include "phxEmber.h"

#include "phxGfxDevice.h"
#include "phxCommandLineArgs.h"

#include "Vulkan/phxVulkanDevice.h"
#include "D3D12/phxD3D12GpuDevice.h"

using namespace phx;
using namespace phx::gfx;

namespace
{
	GpuDevice* m_gpuDevice;
	gfx::GfxBackend m_selectedBackend = gfx::GfxBackend::Dx12;
}

namespace phx::gfx
{
	namespace EmberGfx
	{
		void Initialize(SwapChainDesc const& swapChainDesc, void* windowHandle)
		{
			uint32_t useValidation = 0;
	#if _DEBUG
			// Default to true for debug builds
			useValidation = 1;
	#endif
			CommandLineArgs::GetInteger(L"debug", useValidation);

			if (CommandLineArgs::HasFlag(L"d3d12"))
			{
				m_selectedBackend = gfx::GfxBackend::Dx12;
			}
			else if (CommandLineArgs::HasFlag(L"vulkan"))
			{
				m_selectedBackend = gfx::GfxBackend::Vulkan;
			}

			switch (m_selectedBackend)
			{
			case gfx::GfxBackend::Dx12:
				PHX_CORE_INFO("Creating D3D12 Gpu Device");
				m_gpuDevice = new D3D12GpuDevice();
				break;
			case gfx::GfxBackend::Vulkan:
				PHX_CORE_INFO("Creating Vulkan Gpu Device");
				m_gpuDevice = new platform::VulkanGpuDevice();
				break;
			default:
				throw std::runtime_error("unsupported backed selected");
			}

			m_gpuDevice->Initialize(swapChainDesc, (bool)useValidation, windowHandle);
		}

		void Finalize()
		{
			m_gpuDevice->Finalize();
			delete m_gpuDevice;
			m_gpuDevice = nullptr;
		}

		GpuDevice * GetDevice() { return m_gpuDevice; }
	}
}


