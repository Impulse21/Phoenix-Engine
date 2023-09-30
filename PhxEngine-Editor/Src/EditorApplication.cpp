
#include <PhxEngine/PhxEngine.h>
#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Engine/GameApplication.h>

// -- Add to engine ---
#include <PhxEngine/Core/WorkerThreadPool.h>
#include <PhxEngine/Core/Containers.h>

// -- Temp
#include <PhxEngine/RHI/PhxShaderCompiler.h>

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
		// Compile IMGUI shader to header file
		std::string path = "C:\\Users\\dipao\\source\\repos\\Impulse21\\Phoenix - Engine\\PhxEngine_old\\Shaders\\";
		std::shared_ptr<IFileSystem> fileSystem = CreateNativeFileSystem();

		std::shared_ptr<IFileSystem> relative = CreateRelativeFileSystem(fileSystem, path);
		RHI::ShaderCompiler::CompilerResult result = RHI::ShaderCompiler::Compile({
				.Filename = "ImGuiVS.hlsl",
				.FileSystem = relative.get(),
				.Flags = RHI::ShaderCompiler::CompilerFlags::CreateHeaderFile,
				.ShaderStage = RHI::ShaderStage::Vertex,
				.IncludeDirs = { path }
			});

		assert(result.ErrorMessage.empty());

		result = RHI::ShaderCompiler::Compile({
				.Filename = "ImGuiPS.hlsl",
				.FileSystem = relative.get(),
				.Flags = RHI::ShaderCompiler::CompilerFlags::CreateHeaderFile,
				.ShaderStage = RHI::ShaderStage::Pixel,
				.IncludeDirs = { path }
			});

		assert(result.ErrorMessage.empty());
	}
	// -- Finalize Block ---
	{
		Core::WorkerThreadPool::Finalize();
		Core::ObjectTracker::Finalize();
		assert(0 == SystemMemory::GetMemUsage());
		SystemMemory::Cleanup();
	}
}