#include "pch.h"
#include "phxDeferredReleaseQueue.h"

#include <deque>

using namespace phx;
namespace
{
	std::deque<DeleteItem> m_deleteQueue;
}

namespace phx::DeferredDeleteQueue
{
	void Enqueue(DeleteItem&& deleteItem)
	{
		m_deleteQueue.emplace_back(std::forward<DeleteItem>(deleteItem));
	}

	void ReleaseItems(uint64_t completedFrame)
	{
		while (!m_deleteQueue.empty())
		{
			DeleteItem& deleteItem = m_deleteQueue.front();
			if (deleteItem.Frame < completedFrame)
			{
				deleteItem.DeleteFn();
				m_deleteQueue.pop_front();
			}
			else
			{
				break;
			}
		}
	}
}