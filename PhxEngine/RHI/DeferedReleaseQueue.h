#pragma once

#include <functional>
#include <deque>

namespace PhxEngine::RHI
{
	class DeferedReleaseQueue
	{
	public:
		void Enqueue(std::function<void()> const& releaseFn);
		void Process(uint64_t completedFrame);

	private:
		struct ReleaseItem
		{
			uint64_t Frame;
			std::function<void()> ReleaseFn;
		};

		uint64_t m_processedFrame = 0;
		std::deque<ReleaseItem> m_deleteQueue;
	};
}

