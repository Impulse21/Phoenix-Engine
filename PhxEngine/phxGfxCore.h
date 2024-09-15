#pragma once

#include "phxGfxPlatformDevice.h"
#include "phxGfxPlatformResources.h"

namespace phx
{
	namespace gfx
	{
		void Initialize();
		void Finalize();

		template<typename TDesc, typename TPlatform>
		class DeviceResource
		{
		public:
			virtual ~DeviceResource() = default;

			TPlatform& GetPlatform() { return this->m_platformResource; }
			const TDesc& GetDesc() const { return this->m_desc; }

			explicit operator bool() const
			{
				return !!m_platformResource;
			}

		protected:
			TPlatform m_platformResource;
			TDesc m_desc;
		};

		class SwapChain final : DeviceResource<SwapChainDesc, PlatformSwapChain>
		{
		public:
			SwapChain() = default;

			void Initialize(SwapChainDesc desc);

			void Release()
			{
				this->m_platformResource.Release();
				this->m_desc = {};
			}
		};

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

			void Present(SwapChain const& swapChain)
			{
#if false
				this->m_platform.Present(swapChain.GetPlatform());
#endif
			}

			// -- Getters ---
		public:
			PlatformDevice& GetPlatform() { return this->m_platform; }

		private:
			PlatformDevice m_platform;
		};
	}
}