#pragma once

#include "phxGfxDevice.h"
#include "phxGfxResources.h"

namespace phx
{
	namespace gfx
	{
		void Initialize();
		void Finalize();
	
		class Device
		{
		public:
			inline static Device* Ptr = nullptr;

		public:
			Device() = default;

			void Initialize() { this->m_platform.Initialize(); }
			void Finalize() { this->m_platform.Finalize(); };


			void WaitForIdle()
			{
				this->m_platform.WaitForIdle();
			}

			// -- Factory methods ---
		public:
			void Create(SwapChainDesc const& desc, SwapChain& out)
			{
				// TODO:
			}

		private:
			PlatformDevice m_platform;
		};

		// TODO:
#if false
		extern gDevice;
		extern CommandListManager;
		extern ContextManager;


		// Features
		// Allocators
#endif

	}
}