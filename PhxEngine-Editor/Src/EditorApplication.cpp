
#include <PhxEngine/PhxEngine.h>
#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Engine/GameApplication.h>

// -- Add to engine ---
#include <PhxEngine/Core/WorkerThreadPool.h>

// -- Temp
#include <PhxEngine/Core/StopWatch.h>

#ifdef _MSC_VER // Windows
#include <shellapi.h>
#endif 

using namespace PhxEngine;
using namespace PhxEngine::Core;

class Foo : public Core::Object
{
public:
	Foo() = default;
	uint32_t a = 0;
	uint32_t b = 0;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// -- Engine Start-up ---
	{
		Core::Log::Initialize();
		Core::WorkerThreadPool::Initialize();
	}
	const size_t memSize = PhxGB(1);
	const size_t numElems = memSize / sizeof(Foo);
	auto* data = phx_new_arr(Foo, numElems);

	Core::StopWatch stopWatch;
	stopWatch.Begin();
	for (int i = 0; i < numElems; i++)
	{
		data[i].a = i;
		data[i].b = i + 1;
	}

	TimeStep elapsedTime = stopWatch.Elapsed();

	PHX_LOG_INFO("Fill Array took {0}ms", elapsedTime.GetMilliseconds());

	std:memset(data, 0, memSize);

	stopWatch.Begin();
	WorkerThreadPool::DispatchContext ctx;
	Core::WorkerThreadPool::Dispatch(ctx, numElems, numElems * 0.25, [&](WorkerThreadPool::TaskArgs args) {

			data[args.JobIndex + args.GroupID].a = args.JobIndex + args.GroupIndex;
			data[args.JobIndex + args.GroupID].b =  1;
		});
	Core::WorkerThreadPool::Wait(ctx);

	TimeStep elapsedTime2 = stopWatch.Elapsed();

	PHX_LOG_INFO("Fill Array (Threaded) {0}ms", elapsedTime2.GetMilliseconds());
	// -- Finalize Block ---
	{
		Core::WorkerThreadPool::Finalize();
		Core::ObjectTracker::Finalize();
		assert(0 == SystemMemory::GetMemUsage());
		SystemMemory::Cleanup();
	}
}