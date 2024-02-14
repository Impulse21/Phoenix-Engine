#define NOMINMAX 

#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Core/StringHashTable.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/StopWatch.h>
#include <PhxEngine/Core/RefCountPtr.h>

#include <PhxEngine/Core/Memory.h>

#include "GltfAssetImporter.h"
#include "MeshOptimizationPipeline.h"
#include "MeshletGenerationPipeline.h"
#include <json.hpp>

#include <dstorage.h>
#include <taskflow/taskflow.hpp>
using namespace PhxEngine;

#define ASSERT_SUCCEEDED( hr, ... )				\
        if (FAILED(hr)) {						\
            PHX_LOG_ERROR("HRESULT failed");	\
            __debugbreak(); \
        }

int main(int argc, const char** argv)
{
    Core::Log::Initialize();
	Core::CommandLineArgs::Initialize();
	std::string gltfInput;
	Core::CommandLineArgs::GetString("input", gltfInput);
	tf::Executor workExecutor;

	std::string outputDirectory;
	Core::CommandLineArgs::GetString("output_dir", outputDirectory);

	PHX_LOG_INFO("Baking Assets from %s to %s", gltfInput.c_str(), outputDirectory.c_str());
	std::unique_ptr<Core::IFileSystem> fileSystem = Core::CreateNativeFileSystem();
	
	Pipeline::ImportedObjects importedObjects = {};
	{
		PHX_LOG_INFO("Importing GLTF File: %s", gltfInput.c_str());
		Core::StopWatch elapsedTime;
		Pipeline::GltfAssetImporter gltfImporter;
		gltfImporter.Import(fileSystem.get(), gltfInput, importedObjects);

		PHX_LOG_INFO("Importing GLTF File took %f seconds", elapsedTime.Elapsed().GetSeconds());
	}

	tf::Taskflow taskflow;
	tf::Task MeshPipelineTasks = taskflow.emplace([&importedObjects](tf::Subflow& subflow) {
		tf::Task meshOptimiztionTask = subflow.emplace([&importedObjects]() {
				Pipeline::MeshOptimizationPipeline pipeline;
				Core::LinearAllocator tempAllocator;
				size_t tempBufferSize = PhxGB(1);
				tempAllocator.Initialize(tempBufferSize);
				for (auto& mesh : importedObjects.Meshes)
				{
					pipeline.Optimize(tempAllocator, mesh);
				}
			}).name("Optimize Mesh");

			tf::Task generateMeshletsTask = subflow.emplace([&importedObjects]() {
				Pipeline::MeshletGenerationPipeline pipeline(64, 124);
				for (auto& mesh : importedObjects.Meshes)
				{
					pipeline.GenerateMeshletData(mesh);
				}
				}).name("Generate Meshlets");

			meshOptimiztionTask.precede(generateMeshletsTask);
		});


	// Get the buffer compression interface for DSTORAGE_COMPRESSION_FORMAT_GDEFLATE
	constexpr uint32_t NumCompressionThreads = 6;
	Core::RefCountPtr<IDStorageCompressionCodec> bufferCompression;
	ASSERT_SUCCEEDED(
		DStorageCreateCompressionCodec(DSTORAGE_COMPRESSION_FORMAT_GDEFLATE, NumCompressionThreads, IID_PPV_ARGS(&bufferCompression)));


    return 0;
}
