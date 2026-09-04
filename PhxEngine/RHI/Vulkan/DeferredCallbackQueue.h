#pragma once

#include <functional>
#include <deque>

namespace phx::rhi
{
    template<u32 MAX_FRAMES_INFLIGHT>
    struct DeferredCallbackQueue
	{
		struct DeferredItem
		{
			u64 frame;
			std::function<void()> deferred_func;
		};

		std::deque<DeferredItem> queue;

		void Flush(u64 completed_frame = ~0u)
		{
			while (!queue.empty())
			{
				DeferredItem& deferred_item = queue.front();
				if (deferred_item.frame + MAX_FRAMES_INFLIGHT < completed_frame)
				{
					deferred_item.deferred_func();
					queue.pop_front();
				}
				else
				{
					break;
				}
			}
		}

		void EnqueueDelete(DeferredItem&& item)
		{
			queue.emplace_back(std::forward<DeferredItem>(item));
		}
	};
}