
#include <PhxEngine/PhxEngine.h>
#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Engine/GameApplication.h>

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
	// Engine will use one GB
	Core::Log::Initialize();

	// Test Allocating an object
	Foo* foo = phx_new(Foo);

	assert(0 != SystemMemory::GetMemUsage());
	phx_delete(foo);


	size_t newSize = PhxGB(1);
	auto* data = phx_new_arr(Foo, newSize / sizeof(Foo));
	auto* data1 = phx_new_arr(uint32_t, newSize / sizeof(uint32_t));
	auto* data2 = phx_new_arr(uint32_t, newSize / sizeof(uint32_t));

	size_t data0Size = phx_arr_len(data);
	size_t data1Size = phx_arr_len(data1);
	size_t data2Size = phx_arr_len(data2);

	phx_delete_arr(data);
	phx_delete_arr(data1);
	phx_delete_arr(data2);

	Core::ObjectTracker::Finalize();

	assert(0 == SystemMemory::GetMemUsage());
	SystemMemory::Cleanup();

}