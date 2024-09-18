#pragma once

#include "phxGfxHandle.h"
#include "phxGfxResources.h"

namespace phx
{
	namespace gfx
	{
		void Initialize();
		void Finalize();

		void IdleGpu()
		{

		}

		void SubmitFrame(SwapChain const& swapChain)
		{

		}

		namespace ResourceManager
		{
			bool CreateSwapChain(SwapChainDesc const& desc, SwapChain& out)
			{
				if (out)
				{
					// resize()
				}
				else
				{
					// Create
				}

				return true;
			}

			void Release(SwapChain& out)
			{
				out.Desc = { };
				// TODO: Free Handle
			}
		}
	}
}