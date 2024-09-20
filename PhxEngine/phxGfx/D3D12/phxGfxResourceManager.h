#pragma once

#include "phxGfxCore.h"

#include <deque>
#include <functional>

namespace phx::gfx
{
	class D3D12ResourceManager final : public ResourceManager
	{
	public:
		D3D12ResourceManager() = default;
		~D3D12ResourceManager() = default;

	public:
		void RunGrabageCollection(uint64_t completedFrame = ~0u) override;

	private:
		struct DeleteItem
		{
			uint64_t Frame;
			std::function<void()> DeleteFn;
		};
		std::deque<DeleteItem> m_deleteQueue;
	};
}