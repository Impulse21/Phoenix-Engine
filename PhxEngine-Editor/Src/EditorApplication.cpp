
#include <PhxEngine/PhxEngine.h>
#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Engine/GameApplication.h>

// -- Add to engine ---
#include <PhxEngine/Core/WorkerThreadPool.h>
#include <PhxEngine/Core/Containers.h>

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
	~Foo() { a = ~0u; b = ~0u; }
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
	{
		const size_t memSize = PhxGB(1);
		const size_t numElems = memSize / sizeof(Foo);
		FlexArray<Foo> data;
		data.resize(numElems);

		Core::StopWatch stopWatch;
		stopWatch.Begin();
		for (int i = 0; i < numElems; i++)
		{
			data[i].a = i;
			data[i].b = i + 1;
		}

		TimeStep elapsedTime = stopWatch.Elapsed();

		PHX_LOG_INFO("Fill Array took {0}ms", elapsedTime.GetMilliseconds());

	}
	// -- Finalize Block ---
	{
		Core::WorkerThreadPool::Finalize();
		Core::ObjectTracker::Finalize();
		assert(0 == SystemMemory::GetMemUsage());
		SystemMemory::Cleanup();
	}
}