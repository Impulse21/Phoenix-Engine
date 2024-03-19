#define NOMINMAX 

#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Core/Logger.h>
#include <PhxEngine/Core/StopWatch.h>
#include <PhxEngine/Core/RefCountPtr.h>

#include <PhxEngine/Core/Memory.h>

#include "GltfAssetImporter.h"
#include "MeshOptimizationPipeline.h"
#include "MeshletGenerationPipeline.h"
#include <json.hpp>

#include <dstorage.h>
using namespace PhxEngine;
using namespace nlohmann;

#define ASSERT_SUCCEEDED( hr, ... )				\
        if (FAILED(hr)) {						\
            PHX_LOG_ERROR("HRESULT failed");	\
            __debugbreak(); \
        }

// "{ \"input\" : \"C:\\Users\\dipao\\source\\repos\\Impulse21\\Phoenix-Engine\\Assets\\Main.1_Sponza\\NewSponza_Main_glTF_002.gltf\", \"output_dir\": \"C:\\Users\\dipao\\source\\repos\\Impulse21\\Phoenix-Engine\\Assets\\Main.1_Sponza_Baked\" }"
// "{ \"input\" : \"C:\\Users\\dipao\\source\\repos\\Impulse21\\Phoenix-Engine\\Assets\\Monkey.gltf\", \"output_dir\": \"C:\\Users\\dipao\\source\\repos\\Impulse21\\Phoenix-Engine\\Assets\" }"
int main(int argc, const char** argv)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	assert(SUCCEEDED(hr));

	Logger::Startup();
	if (argc == 0)
	{
		PHX_LOG_ERROR("Input json is expected");
		return -1;
	}

	json inputSettings = json::parse(argv[1]);

	const std::string inputTag = "input";
	const std::string outputTag = "output_dir";
	if (!inputSettings.contains(inputTag))
	{
		PHX_LOG_ERROR("Input is required");
		return -1;
	}

	if (!inputSettings.contains(outputTag))
	{
		PHX_LOG_ERROR("Out directory");
		return -1;
	}

	const std::string& gltfInput = inputSettings[inputTag];
	const std::string& outputDirectory = inputSettings[outputTag];

	PHX_LOG_INFO("Baking Assets from %s to %s", gltfInput.c_str(), outputDirectory.c_str());
	std::unique_ptr<IFileSystem> fileSystem = FileSystemFactory::CreateNativeFileSystem();

	StopWatch elapsedTime;
	Pipeline::ImportedObjects importedObjects = {};
	{
		PHX_LOG_INFO("Importing GLTF File: %s", gltfInput.c_str());
		elapsedTime.Begin();
		Pipeline::GltfAssetImporter gltfImporter;
		gltfImporter.Import(fileSystem.get(), gltfInput, importedObjects);

		PHX_LOG_INFO("Importing GLTF File took %f seconds", elapsedTime.Elapsed().GetSeconds());
	}

	//  tf::Taskflow texturePipelineTask = taskflow.emplace([&importedObjects](tf::sub))

	StackAllocator tempAllocator(PhxGB(1));

	Pipeline::MeshOptimizationPipeline optmizationPipeline;
	for (auto& mesh : importedObjects.Meshes)
	{
		PHX_LOG_INFO("Optmizing '%s' mesh ", mesh.Name.c_str());
		elapsedTime.Begin();
		optmizationPipeline.Optimize(tempAllocator, mesh);
		PHX_LOG_INFO("Optmizing '%s' mesh took %f seconds", mesh.Name.c_str(), elapsedTime.Elapsed().GetSeconds());
	}

	elapsedTime.Begin();
	Pipeline::MeshletGenerationPipeline pipeline(64, 124);
	for (auto& mesh : importedObjects.Meshes)
	{
		PHX_LOG_INFO("Generating Meshlets for '%s' mesh ", mesh.Name.c_str());
		elapsedTime.Begin();
		pipeline.GenerateMeshletData(mesh);
		PHX_LOG_INFO("Generating Meshlets for '%s' mesh took %f seconds", mesh.Name.c_str(), elapsedTime.Elapsed().GetSeconds());
	}

	// Create and save Drawable.
	
	// Get the buffer compression interface for DSTORAGE_COMPRESSION_FORMAT_GDEFLATE
	constexpr uint32_t NumCompressionThreads = 6;
	RefCountPtr<IDStorageCompressionCodec> bufferCompression;
	ASSERT_SUCCEEDED(
		DStorageCreateCompressionCodec(DSTORAGE_COMPRESSION_FORMAT_GDEFLATE, NumCompressionThreads, IID_PPV_ARGS(&bufferCompression)));


    return 0;
}
