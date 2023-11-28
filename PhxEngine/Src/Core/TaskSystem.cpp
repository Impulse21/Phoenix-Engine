
#include <PhxEngine/Core/TaskSystem.h>

using namespace PhxEngine::Core;

namespace
{
	tf::Executor m_taskExecutor;

}

void TaskSystem::WaitForIdle()
{
	m_taskExecutor.wait_for_all();
}

tf::Executor& PhxEngine::Core::TaskSystem::GetExecutor()
{
	return m_taskExecutor;
}