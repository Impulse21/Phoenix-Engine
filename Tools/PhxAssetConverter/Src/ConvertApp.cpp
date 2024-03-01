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
#include "TextureConvertPipeline.h"
#include "ResourceExporter.h"
#include "ICompressor.h"
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
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	assert(SUCCEEDED(hr));

    Core::Log::Initialize();
	Core::CommandLineArgs::Initialize();
	std::string gltfInput;
	Core::CommandLineArgs::GetString("input", gltfInput);
	tf::Executor workExecutor;

	std::string outputDirectory;
	Core::CommandLineArgs::GetString("output_dir", outputDirectory);

	PHX_LOG_INFO("Baking Assets from %s to %s", gltfInput.c_str(), outputDirectory.c_str());
	std::shared_ptr<Core::IFileSystem> fileSystem = Core::CreateNativeFileSystem();
	std::unique_ptr<Core::IFileSystem> outputFileSystem = Core::CreateRelativeFileSystem(fileSystem, outputDirectory);

	Pipeline::ICompressor* compresor = nullptr;

	Pipeline::ImportedObjects importedObjects = {};
	{
		PHX_LOG_INFO("Importing GLTF File: %s", gltfInput.c_str());
		Core::StopWatch elapsedTime;
		Pipeline::GltfAssetImporter gltfImporter;
		gltfImporter.Import(fileSystem.get(), gltfInput, importedObjects);

		PHX_LOG_INFO("Importing GLTF File took %f seconds", elapsedTime.Elapsed().GetSeconds());
	}

	tf::Taskflow taskflow;
	tf::Task meshPipelineTasks = taskflow.emplace([&importedObjects, compresor, &outputFileSystem](tf::Subflow& subflow) {
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

		tf::Task resourceTask = subflow.emplace([&importedObjects, compresor, &outputFileSystem]() {
			for (auto& mesh : importedObjects.Meshes)
			{
				std::stringstream strStream;
				Pipeline::MeshResourceExporter::Export(strStream, compresor, mesh);
				std::string fileData = strStream.str();
				outputFileSystem->WriteFile(mesh.Name + ".pmesh", Core::Span<char>(fileData.c_str(), fileData.length()));
			}
		}).name("Resource Mesh");

		meshOptimiztionTask.precede(generateMeshletsTask);
	}).name("Mesh pipeline");

	tf::Task texturePipelineTask = taskflow.emplace([&importedObjects, &fileSystem, compresor, &outputFileSystem](tf::Subflow& subflow) {
		tf::Task TextureConvertTask = subflow.emplace([&importedObjects, &fileSystem]() {
				Pipeline::TextureConvertPipeline pipeline(fileSystem.get());
				for (auto& texture : importedObjects.Textures)
				{
					pipeline.Convert(texture, Pipeline::TexConversionFlags::DefaultBC);
				}
			}).name("Convert Textures");

		tf::Task resourceTexturesTask = subflow.emplace([&importedObjects, &fileSystem, compresor, &outputFileSystem]() {
			for (auto& texture : importedObjects.Textures)
			{
				std::stringstream strStream;
				Pipeline::TextureResourceExporter::Export(strStream, compresor, texture);
				std::string fileData = strStream.str();
				outputFileSystem->WriteFile(texture.Name + ".ptex", Core::Span<char>(fileData.c_str(), fileData.length()));
			}
		}).name("Resource Textures");
	}).name("Texture Pipeline");

	tf::Task materialPipelineTask = taskflow.emplace([&importedObjects, &fileSystem, compresor, &outputFileSystem](tf::Subflow& subflow) {
		tf::Task resourceTexturesTask = subflow.emplace([&importedObjects, &fileSystem, compresor, &outputFileSystem]() {
			for (auto& mtl : importedObjects.Materials)
			{
				std::stringstream strStream;
				Pipeline::MaterialResourceExporter::Export(strStream, compresor, mtl);
				std::string fileData = strStream.str();
				outputFileSystem->WriteFile(mtl.Name + ".pmat", Core::Span<char>(fileData.c_str(), fileData.length()));
			}
		}).name("Resource Materials");
	}).name("Material Pipeline");

	//  tf::Taskflow texturePipelineTask = taskflow.emplace([&importedObjects](tf::sub))

	// Get the buffer compression interface for DSTORAGE_COMPRESSION_FORMAT_GDEFLATE
	constexpr uint32_t NumCompressionThreads = 6;
	Core::RefCountPtr<IDStorageCompressionCodec> bufferCompression;
	ASSERT_SUCCEEDED(
		DStorageCreateCompressionCodec(DSTORAGE_COMPRESSION_FORMAT_GDEFLATE, NumCompressionThreads, IID_PPV_ARGS(&bufferCompression)));


    return 0;
}
