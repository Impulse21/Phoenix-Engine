#include <PhxCore/Base.h>
#include <PhxCore/CommandLineArgs.h>
#include <PhxCore/Log.h>
#include <PhxCore/VFS.h>

#include <wrl.h>
#include <dstorage.h>
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <yaml-cpp/yaml.h>

using namespace Microsoft::WRL;
// "{ \"input\" : \"Main.1_Sponza\\NewSponza_Main_glTF_002.gltf\", \"output_file\": \"Sponza.phxarc", \"compression\" : \"GDeflate\" }"

constexpr const char* kTestInput= R"(
	input: "C:\Users\dipao\OneDrive\Documents\Art\main1_sponzas\NewSponza_Main_glTF_003.gltf"
	output_file: "Sponza.phxpak"
	compression: "GDeflate")";

namespace phx
{
	enum class CompressionType : uint16_t
	{
		None = 0,
		GDeflate = 1,
	};
	class IFileSystem;

	enum TexConversionFlags
	{
		kSRGB = BIT(0),   // Texture contains sRGB colors
		kPreserveAlpha = BIT(1),   // Keep four channels
		kNormalMap = BIT(2),   // Texture contains normals
		kBumpToNormal = BIT(3),   // Generate a normal map from a bump map
		kDefaultBC = BIT(4),   // Apply standard block compression (BC1-5)
		kQualityBC = BIT(5),   // Apply quality block compression (BC6H/7)
		kFlipVertical = BIT(6),
	};

	inline uint8_t TextureOptions(bool sRGB, bool hasAlpha = false, bool invertY = false)
	{
		return (sRGB ? kSRGB : 0) | (hasAlpha ? kPreserveAlpha : 0) | (invertY ? kFlipVertical : 0);
	}

	namespace TextureCompiler
	{
		// std::unique_ptr<ScratchImage> BuildDDS(std::string const& filename, uint32_t flags);
		void CompileOnDemand(IFileSystem& fs, std::string const& filename, uint32_t flags);
	}
}

namespace
{
	constexpr const char* kInputTag = "input";
	constexpr const char* kOutputTag = "output_file";
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

    YAML::Node config = YAML::Load(kTestInput);

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
	auto outputFilename = config[kOutputTag].as<std::string>();

	bool useGDeflate = false;
	if (config[kCompressionTag])
	{
		auto compressionStr = config[kCompressionTag].as<std::string>();
		useGDeflate = compressionStr == "gdeflate";
	}

	PHX_INFO("Creating Phoenix Pack File '%s' from '%s'", outputFilename, gltfInput);
	std::filesystem::path gltfInputPath(gltfInput);
	gltfInputPath.make_preferred();
	std::unique_ptr<phx::IFileSystem> fs = phx::FileSystemFactory::CreateRelativeFileSystem(phx::FileSystemFactory::CreateNativeFileSystem(), gltfInputPath.parent_path());

	{
		// Load GLF File into memory
		CgltfContext context =
		{
			.FileSystem = fs.get(),
			.Blobs = {}
		};

		cgltf_options options = { };
		options.file.read = &CgltfReadFile;
		options.file.release = &CgltfReleaseFile;
		options.file.user_data = &context;


		std::unique_ptr<phx::IBlob> blob = fs->ReadFile(gltfInputPath);
		if (!blob)
		{
			PHX_ERROR("Couldn't Read file %s", gltfInput);
			return false;
		}

		cgltf_data* gltfData = nullptr;
		cgltf_result res = cgltf_parse(&options, blob->Data(), blob->Size(), &gltfData);
		if (res != cgltf_result_success)
		{
			PHX_ERROR("Couldn't load glTF file %s", gltfInput);
			return false;
		}

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


		phx::CompressionType compression = phx::CompressionType::None;
		if (useGDeflate)
			compression = phx::CompressionType::GDeflate;

		(void)compression;
		bool useBC = true;
		phx::TexConversionFlags extraTextureFlags{};
		if (useBC)
		{
			extraTextureFlags = static_cast<phx::TexConversionFlags>(extraTextureFlags | phx::kDefaultBC);
		}

#if false
		uint32_t stagingBufferSize = 256_MiB;
		std::filesystem::path outputPath(outputFilename);
		outputPath.make_preferred();
#endif
	}

    return 0;
}