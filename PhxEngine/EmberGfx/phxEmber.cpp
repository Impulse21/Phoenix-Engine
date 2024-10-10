#include "pch.h"
#include "phxEmber.h"

#include "phxGfxDevice.h"
#include "phxCommandLineArgs.h"

using namespace phx;
using namespace phx::gfx;

namespace
{
	GpuDevice* m_gpuDevice;
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

			m_gpuDevice = new platform::VulkanGpuDevice();

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


