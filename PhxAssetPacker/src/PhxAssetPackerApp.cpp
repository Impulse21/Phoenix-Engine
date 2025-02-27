#include <PhxCore/Base.h>
#include <PhxCore/CommandLineArgs.h>
#include <PhxCore/Log.h>
#include <PhxCore/VFS.h>
#include <PhxCore/SystemTime.h>
#include <PhxCore/BinaryBuilder.h>
#include <PhxCore/Span.h>

#include <PhxResource/ChunkFileFormat.h>
#include <PhxResource/PakFileFormat.h>

#include "TextureConverter.h"
#include "GltfImporter.h"
#include "PakFileBuilder.h"

#include <wrl.h>
#include <dstorage.h>
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <yaml-cpp/yaml.h>
#include <PhxCore/StringUtils.h>
#include "PhxCore/ThreadPool.h"

using namespace phx;
using namespace Microsoft::WRL;

// Args for Laptop: -config "../../PhxAssetPacker/test_config_laptop.yaml"
// Args For Matrix: -config "../../PhxAssetPacker/test_config_matrix.yaml"
namespace
{
	constexpr const char* kInputTag = "input";
	constexpr const char* kOutputTag = "output_dir";
	constexpr const char* kCompressionTag = "compression";

	ComPtr<IDStorageCompressionCodec> g_bufferCompression;
	struct CgltfContext
	{
		phx::IFileSystem* FileSystem;
		std::vector<std::shared_ptr<phx::IBlob>> Blobs;
	};

	cgltf_result CgltfReadFile(const cgltf_memory_options*, const cgltf_file_options* file_options, const char* path, cgltf_size* size, void** Data)
	{
		CgltfContext* context = (CgltfContext*)file_options->user_data;

		std::unique_ptr<phx::IBlob> dataBlob = context->FileSystem->ReadFile(path);
		if (!dataBlob)
		{
			return cgltf_result_file_not_found;
		}

		if (size)
		{
			*size = dataBlob->Size();
		}

		if (Data)
		{
			*Data = (void*)dataBlob->Data();  // NOLINT(clang-diagnostic-cast-qual)
		}

		context->Blobs.push_back(std::move(dataBlob));

		return cgltf_result_success;
	}

	void CgltfReleaseFile(
		const struct cgltf_memory_options*,
		const struct cgltf_file_options*,
		void*)
	{
		// do nothing
	}

}

int wmain(int argc, wchar_t** argv)
{
	phx::Log::Initialize();
	if (argc == 0)
	{
		PHX_INFO("YAML config expected");
		return -1;
	}

	phx::CommandLineArgs::Initialize(argc, argv);
	phx::ThreadPool::Initialize();

	std::wstring wConfigFile;
	phx::CommandLineArgs::GetString(L"config", wConfigFile);

	std::string configFile;
	StringConvert(wConfigFile, configFile);

	YAML::Node config = YAML::LoadFile(configFile.c_str());

	if (!config[kInputTag])
	{
		PHX_ERROR("Input is required");
		return -1;
	}

	if (!config[kOutputTag])
	{
		PHX_ERROR("Missing output file tag");
		return -1;
	}

	auto gltfInput = config[kInputTag].as<std::string>();
	auto outputDir = config[kOutputTag].as<std::string>();

	bool useGDeflate = false;
	if (config[kCompressionTag])
	{
		auto compressionStr = config[kCompressionTag].as<std::string>();
		useGDeflate = compressionStr == "gdeflate";
	}

	std::filesystem::path gltfInputPath(gltfInput);
	gltfInputPath.make_preferred();

	std::string outputFilename = std::format("{}.{}", gltfInputPath.stem().generic_string().c_str(), "phxpak");
	PHX_INFO("Creating Phoenix Pack File '{0}' from '{1}'", outputFilename.c_str(), gltfInput);

	std::shared_ptr<phx::IFileSystem> nativeFS = phx::FileSystemFactory::CreateNativeFileSystem();
	std::unique_ptr<phx::IFileSystem> inputFS = phx::FileSystemFactory::CreateRelativeFileSystem(nativeFS, gltfInputPath.parent_path());

	std::filesystem::create_directories(outputDir);
	std::unique_ptr<phx::IFileSystem> outputFS = phx::FileSystemFactory::CreateRelativeFileSystem(nativeFS, outputDir);

	{
		// Load GLF File into memory
		CgltfContext context =
		{
			.FileSystem = nativeFS.get(),
			.Blobs = {}
		};

		cgltf_options options = { };
		options.file.read = &CgltfReadFile;
		options.file.release = &CgltfReleaseFile;
		options.file.user_data = &context;


		std::unique_ptr<phx::IBlob> blob = nativeFS->ReadFile(gltfInputPath);
		if (!blob)
		{
			PHX_ERROR("Couldn't Read file {0}", gltfInput);
			return false;
		}

		cgltf_data* gltfData = nullptr;
		cgltf_result res = cgltf_parse(&options, blob->Data(), blob->Size(), &gltfData);
		if (res != cgltf_result_success)
		{
			PHX_ERROR("Couldn't load glTF file {0}", gltfInput);
			return false;
		}

		res = cgltf_load_buffers(&options, gltfData, gltfInput.c_str());
		if (res != cgltf_result_success)
		{
			PHX_ERROR("Couldn't load glTF Binary data {0}", gltfInput.c_str());
			return false;
		}

		phx::CpuTimer timer;
		std::vector<phx::MeshData> importedMeshes = phx::GltfMeshImporter::Import(gltfData);
		PHX_INFO(
			"Imported {0} Meshes in {1} ms",
			importedMeshes.size(),
			timer.Elapsed().GetMilliseconds());

		if (useGDeflate)
		{
			// Get the buffer compression interface for DSTORAGE_COMPRESSION_FORMAT_GDEFLATE
			// The number 6 comes form mini engine - I don't know the reason why this number was selected.
			constexpr uint32_t NumCompressionThreads = 6;
			SUCCEEDED(
				DStorageCreateCompressionCodec(
					DSTORAGE_COMPRESSION_FORMAT_GDEFLATE,
					NumCompressionThreads,
					IID_PPV_ARGS(&g_bufferCompression)));
		}


		cgltf_free(gltfData);

		phx::FileFormat::CompressionType compression = phx::FileFormat::CompressionType::None;
		if (useGDeflate)
			compression = phx::FileFormat::CompressionType::GDeflate;

		(void)compression;
		bool useBC = true;
		phx::TexConversionFlags extraTextureFlags{};
		if (useBC)
		{
			extraTextureFlags = static_cast<phx::TexConversionFlags>(extraTextureFlags | phx::kDefaultBC);
		}

		// uint32_t stagingBufferSize = 256_MiB;
		std::filesystem::path outputPath(outputFilename);
		outputPath.make_preferred();
		timer.Begin();

		std::vector<CompiledMeshResource> meshChunkFiles(importedMeshes.size());
		for (size_t i = 0; i < meshChunkFiles.size(); i++)
		{
			CompiledMeshResource& blob = meshChunkFiles[i];
			const MeshData& meshData = importedMeshes[i];

#if true
			ThreadPool::SubmitTask([meshData, &blob]() {
				MeshResourceCompiler::Compile(meshData, blob);
			});
#else
			MeshResourceCompiler::Compile(meshData, blob);
#endif
		}
		PHX_INFO(
			"Compiled {0} Mesh Chunk files in {1} ms",
			meshChunkFiles.size(),
			timer.Elapsed().GetMilliseconds());

		// Build an order map of chunk Entires
		std::vector<std::pair<std::string, IBlob*>> assetEntries;
		for (size_t i = 0; i < meshChunkFiles.size(); i++)
		{
			auto& meshFile = meshChunkFiles[i];
			PHX_ASSERT(meshFile.File);
			assetEntries.push_back(std::make_pair(meshFile.FileName, meshFile.File.get()));
		}

		// Lets save a pack file :)
		timer.Begin();
		phx::PakFileBuilder pakBuilder = phx::PakFileBuilder()
			.AddFiles(assetEntries);

		std::unique_ptr<phx::IBlob> pakFileData = pakBuilder.Build();
		outputFS->WriteFile(outputFilename, pakFileData.get());

		// Write out mesh Chunk Files
		// Write out Chunk Data
		// Calculate entry Offsets?
		PHX_INFO("Exporting Archive file '{0}' took {1} seconds", outputFilename, timer.Elapsed().GetSeconds());
	}

    return 0;
}