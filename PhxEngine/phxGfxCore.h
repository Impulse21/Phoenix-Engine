#pragma once

#include "phxGfxHandle.h"
#include "phxGfxResources.h"

#include "phxGfxPlatformImpl.h"

namespace phx
{
	namespace gfx
	{
		void Initialize();
		void Finalize();

		void IdleGpu()
		{
			platform::IdleGpu();
		}

		void SubmitFrame(SwapChain const& swapChain)
		{
			platform::SubmitFrame(swapChain.Handle);
		}

		namespace ResourceManager
		{
			void CreateSwapChain(SwapChainDesc const& desc, SwapChain& out)
			{
				out.Desc = desc;
				platform::ResourceManger::CreateSwapChain(desc, out.Handle);
			}

			void Release(SwapChain& out)
			{
				out.Desc = { };
				// TODO: Free Handle
			}
		}
	}
}