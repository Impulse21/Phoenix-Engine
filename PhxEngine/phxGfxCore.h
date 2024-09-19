#pragma once

#include "phxGfxHandle.h"
#include "phxGfxResources.h"

#include "phxGfxPlatformImpl.h"

namespace phx
{
	namespace gfx
	{
		class CommandContext
		{
		public:

		private:
			platform::CommandContext m_context;
		};

		void Initialize();
		void Finalize();

		inline void IdleGpu()
		{
			platform::IdleGpu();
		}

		inline void SubmitFrame(SwapChain const& swapChain)
		{
			platform::SubmitFrame(swapChain.Handle);
		}

		CommandContext& BeginContext()
		{

		}

		namespace ResourceManager
		{
			inline void CreateSwapChain(SwapChainDesc const& desc, SwapChain& out)
			{
				out.Desc = desc;
				platform::ResourceManger::CreateSwapChain(desc, out.Handle);
			}

			inline void Release(SwapChain& out)
			{
				out.Desc = { };
				// TODO: Free Handle
			}
		}
	}
}