
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

	PHX_LOG_INFO("Intiializing 1GB of Engine Memory");
	Core::SystemMemory::Initialize({ .MaximumDynamicSize = PhxGB(1) });

	// Test Allocating an object
	Foo* foo = phx_new(Foo);

	phx_delete(foo);
	
	size_t newSize = 16;
	auto* data = phx_new_arr(Foo, newSize);
	auto* data1 = phx_new_arr(uint32_t, newSize);
	auto* data2 = phx_new_arr(uint32_t, newSize);

	size_t data0Size = phx_arr_len(data);
	size_t data1Size = phx_arr_len(data1);
	size_t data2Size = phx_arr_len(data2);

	phx_delete_arr(data);
	phx_delete_arr(data1);
	phx_delete_arr(data2);

	newSize *= 2;
	auto* newData = phx_new_arr(Foo, newSize);
	auto* newData1 = phx_new_arr(uint32_t, newSize);
	auto* newData2 = phx_new_arr(uint32_t, newSize);

	data = newData;
	data1 = newData1;
	data2 = newData2;

	Core::ObjectTracker::CleanUp();

	Core::SystemMemory::Finalize();

}