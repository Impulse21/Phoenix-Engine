#pragma once

#include <taskflow/taskflow.hpp>

namespace PhxEngine::Core
{
	namespace TaskSystem
	{
		void WaitForIdle();

		tf::Executor& GetExecutor();


		template <typename F>
		void RunSilentTask(F&& task)
		{
			GetExecutor().silent_async(std::move(task));
		}

	}
}