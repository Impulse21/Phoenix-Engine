#pragma once

#include <functional>
#include <deque>

namespace PhxEngine::RHI
{
	class DeferedReleaseQueue
	{
	public:
		static void Enqueue(std::function<void()> const& releaseFn);

		static void Process(uint64_t completedFrame);

	private:
		struct ReleaseItem
		{
			uint64_t Frame;
			std::function<void()> ReleaseFn;
		};

		static uint64_t m_processedFrame;
		static std::deque<ReleaseItem> m_deleteQueue;
	};
}

