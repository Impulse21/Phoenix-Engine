#include "DeferedReleaseQueue.h"

using namespace PhxEngine::RHI;

uint64_t DeferedReleaseQueue::m_processedFrame = 0;
std::deque<DeferedReleaseQueue::ReleaseItem> DeferedReleaseQueue::m_deleteQueue;

void PhxEngine::RHI::DeferedReleaseQueue::Enqueue(std::function<void()> const& releaseFn)
{
	m_deleteQueue.push_back(
		{
			.Frame = m_processedFrame,
			.ReleaseFn = releaseFn
		});
}

void PhxEngine::RHI::DeferedReleaseQueue::Process(uint64_t completedFrame)
{
	while (!m_deleteQueue.empty())
	{
		ReleaseItem& item = m_deleteQueue.front();
		if (item.Frame < completedFrame)
		{
			item.ReleaseFn();
			m_deleteQueue.pop_front();
		}
		else
		{
			break;
		}
	}

	m_processedFrame = completedFrame;
}
