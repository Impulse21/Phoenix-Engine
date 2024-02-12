#define NOMINMAX 

#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Core/StringHashTable.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/StopWatch.h>

#include <PhxEngine/Core/Memory.h>

#include "GltfAssetImporter.h"
#include "MeshOptimizationPipeline.h"
#include <json.hpp>

using namespace PhxEngine;

int main(int argc, const char** argv)
{
    Core::Log::Initialize();
	Core::CommandLineArgs::Initialize();
	std::string gltfInput;
	Core::CommandLineArgs::GetString("input", gltfInput);


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

	// Process Mesh Data
	Pipeline::MeshOptimizationPipeline pipeline;

	Core::LinearAllocator tempAllocator;
	size_t tempBufferSize = PhxGB(1);
	tempAllocator.Initialize(tempBufferSize);
	for (auto& mesh : importedObjects.Meshes)
	{
		pipeline.Optimize(tempAllocator, mesh);
	}

    return 0;
}
