#pragma once

#include "phxGfxCore.h"

#include <deque>
#include <functional>

namespace phx::gfx
{
	class D3D12ResourceManager final : public ResourceManager
	{
	public:
		inline static D3D12ResourceManager* Impl() { return Singleton; }
	public:
		D3D12ResourceManager()
		{
			Singleton = this;
		}
		~D3D12ResourceManager()
		{
			Singleton = nullptr;
		};

	public:
		void RunGrabageCollection(uint64_t completedFrame = ~0u) override;

	private:
		inline static D3D12ResourceManager* Singleton = nullptr;

		struct DeleteItem
		{
			uint64_t Frame;
			std::function<void()> DeleteFn;
		};
		std::deque<DeleteItem> m_deleteQueue;
	};
}