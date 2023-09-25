
#include <PhxEngine/PhxEngine.h>
#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Engine/GameApplication.h>

// -- Add to engine ---
#include <PhxEngine/Core/WorkerThreadPool.h>

#ifdef _MSC_VER // Windows
#include <shellapi.h>
#endif 

using namespace PhxEngine;
using namespace PhxEngine::Core;

class Foo : public Core::Object
{
public:
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

	// Test Allocating an object
	Foo* foo = phx_new(Foo);

	assert(0 != SystemMemory::GetMemUsage());
	phx_delete(foo);

	const size_t memSize = PhxGB(1);
	const size_t numElems = memSize / sizeof(Foo);
	auto* data = phx_new_arr(Foo, numElems);

	for (int i = 0; i < numElems; i++)
	{

	}
	Core::WorkerThreadPool::Dispatch()

	// -- Finalize Block ---
	{
		Core::ObjectTracker::Finalize();
		Core::WorkerThreadPool::Finalize();
		assert(0 == SystemMemory::GetMemUsage());
		SystemMemory::Cleanup();
	}
}