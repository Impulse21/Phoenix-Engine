// PhxArchive.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>

#include <dstorage.h>
#include <fstream>
#include <assert.h>

#include <Core/phxMemory.h>
#include <Core/phxLog.h>
#include <Core/phxStopWatch.h>
#include <Core/phxVirtualFileSystem.h>
#include <Resource/phxArcFileFormat.h>
#include <Core/phxBinaryBuilder.h>
#include <RHI/phxRHI.h>
#include <RHI/D3D12/d3dx12.h>

#include "phxTextureConvert.h"
#include "3rdParty/nlohmann/json.hpp"
#include "phxModelImporter.h"
#include "phxModelImporterGltf.h"
#include <wrl.h>

using namespace phx;
using namespace phx::core;
using namespace phx::arc;
using namespace Microsoft::WRL;
using namespace nlohmann;

namespace
{
	struct FormatMapping
	{
		rhi::Format PhxRHIFormat;
		DXGI_FORMAT DxgiFormat;
		uint32_t BitsPerPixel;
	};
	const FormatMapping g_FormatMappings[] = {
	{ rhi::Format::UNKNOWN,              DXGI_FORMAT_UNKNOWN,                0 },
	{ rhi::Format::R8_UINT,              DXGI_FORMAT_R8_UINT,                8 },
	{ rhi::Format::R8_SINT,              DXGI_FORMAT_R8_SINT,                8 },
	{ rhi::Format::R8_UNORM,             DXGI_FORMAT_R8_UNORM,               8 },
	{ rhi::Format::R8_SNORM,             DXGI_FORMAT_R8_SNORM,               8 },
	{ rhi::Format::RG8_UINT,             DXGI_FORMAT_R8G8_UINT,              16 },
	{ rhi::Format::RG8_SINT,             DXGI_FORMAT_R8G8_SINT,              16 },
	{ rhi::Format::RG8_UNORM,            DXGI_FORMAT_R8G8_UNORM,             16 },
	{ rhi::Format::RG8_SNORM,            DXGI_FORMAT_R8G8_SNORM,             16 },
	{ rhi::Format::R16_UINT,             DXGI_FORMAT_R16_UINT,               16 },
	{ rhi::Format::R16_SINT,             DXGI_FORMAT_R16_SINT,               16 },
	{ rhi::Format::R16_UNORM,            DXGI_FORMAT_R16_UNORM,              16 },
	{ rhi::Format::R16_SNORM,            DXGI_FORMAT_R16_SNORM,              16 },
	{ rhi::Format::R16_FLOAT,            DXGI_FORMAT_R16_FLOAT,              16 },
	{ rhi::Format::BGRA4_UNORM,          DXGI_FORMAT_B4G4R4A4_UNORM,         16 },
	{ rhi::Format::B5G6R5_UNORM,         DXGI_FORMAT_B5G6R5_UNORM,           16 },
	{ rhi::Format::B5G5R5A1_UNORM,       DXGI_FORMAT_B5G5R5A1_UNORM,         16 },
	{ rhi::Format::RGBA8_UINT,           DXGI_FORMAT_R8G8B8A8_UINT,          32 },
	{ rhi::Format::RGBA8_SINT,           DXGI_FORMAT_R8G8B8A8_SINT,          32 },
	{ rhi::Format::RGBA8_UNORM,          DXGI_FORMAT_R8G8B8A8_UNORM,         32 },
	{ rhi::Format::RGBA8_SNORM,          DXGI_FORMAT_R8G8B8A8_SNORM,         32 },
	{ rhi::Format::BGRA8_UNORM,          DXGI_FORMAT_B8G8R8A8_UNORM,         32 },
	{ rhi::Format::SRGBA8_UNORM,         DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,    32 },
	{ rhi::Format::SBGRA8_UNORM,         DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,    32 },
	{ rhi::Format::R10G10B10A2_UNORM,    DXGI_FORMAT_R10G10B10A2_UNORM,      32 },
	{ rhi::Format::R11G11B10_FLOAT,      DXGI_FORMAT_R11G11B10_FLOAT,        32 },
	{ rhi::Format::RG16_UINT,            DXGI_FORMAT_R16G16_UINT,            32 },
	{ rhi::Format::RG16_SINT,            DXGI_FORMAT_R16G16_SINT,            32 },
	{ rhi::Format::RG16_UNORM,           DXGI_FORMAT_R16G16_UNORM,           32 },
	{ rhi::Format::RG16_SNORM,           DXGI_FORMAT_R16G16_SNORM,           32 },
	{ rhi::Format::RG16_FLOAT,           DXGI_FORMAT_R16G16_FLOAT,           32 },
	{ rhi::Format::R32_UINT,             DXGI_FORMAT_R32_UINT,               32 },
	{ rhi::Format::R32_SINT,             DXGI_FORMAT_R32_SINT,               32 },
	{ rhi::Format::R32_FLOAT,            DXGI_FORMAT_R32_FLOAT,              32 },
	{ rhi::Format::RGBA16_UINT,          DXGI_FORMAT_R16G16B16A16_UINT,      64 },
	{ rhi::Format::RGBA16_SINT,          DXGI_FORMAT_R16G16B16A16_SINT,      64 },
	{ rhi::Format::RGBA16_FLOAT,         DXGI_FORMAT_R16G16B16A16_FLOAT,     64 },
	{ rhi::Format::RGBA16_UNORM,         DXGI_FORMAT_R16G16B16A16_UNORM,     64 },
	{ rhi::Format::RGBA16_SNORM,         DXGI_FORMAT_R16G16B16A16_SNORM,     64 },
	{ rhi::Format::RG32_UINT,            DXGI_FORMAT_R32G32_UINT,            64 },
	{ rhi::Format::RG32_SINT,            DXGI_FORMAT_R32G32_SINT,            64 },
	{ rhi::Format::RG32_FLOAT,           DXGI_FORMAT_R32G32_FLOAT,           64 },
	{ rhi::Format::RGB32_UINT,           DXGI_FORMAT_R32G32B32_UINT,         96 },
	{ rhi::Format::RGB32_SINT,           DXGI_FORMAT_R32G32B32_SINT,         96 },
	{ rhi::Format::RGB32_FLOAT,          DXGI_FORMAT_R32G32B32_FLOAT,        96 },
	{ rhi::Format::RGBA32_UINT,          DXGI_FORMAT_R32G32B32A32_UINT,      128 },
	{ rhi::Format::RGBA32_SINT,          DXGI_FORMAT_R32G32B32A32_SINT,      128 },
	{ rhi::Format::RGBA32_FLOAT,         DXGI_FORMAT_R32G32B32A32_FLOAT,     128 },
	{ rhi::Format::D16,                  DXGI_FORMAT_R16_UNORM,              16 },
	{ rhi::Format::D24S8,                DXGI_FORMAT_R24_UNORM_X8_TYPELESS,  32 },
	{ rhi::Format::X24G8_UINT,           DXGI_FORMAT_X24_TYPELESS_G8_UINT,   32 },
	{ rhi::Format::D32,                  DXGI_FORMAT_R32_FLOAT,              32 },
	{ rhi::Format::D32S8,                DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, 64 },
	{ rhi::Format::X32G8_UINT,           DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,  64 },
	{ rhi::Format::BC1_UNORM,            DXGI_FORMAT_BC1_UNORM,              4 },
	{ rhi::Format::BC1_UNORM_SRGB,       DXGI_FORMAT_BC1_UNORM_SRGB,         4 },
	{ rhi::Format::BC2_UNORM,            DXGI_FORMAT_BC2_UNORM,              8 },
	{ rhi::Format::BC2_UNORM_SRGB,       DXGI_FORMAT_BC2_UNORM_SRGB,         8 },
	{ rhi::Format::BC3_UNORM,            DXGI_FORMAT_BC3_UNORM,              8 },
	{ rhi::Format::BC3_UNORM_SRGB,       DXGI_FORMAT_BC3_UNORM_SRGB,         8 },
	{ rhi::Format::BC4_UNORM,            DXGI_FORMAT_BC4_UNORM,              4 },
	{ rhi::Format::BC4_SNORM,            DXGI_FORMAT_BC4_SNORM,              4 },
	{ rhi::Format::BC5_UNORM,            DXGI_FORMAT_BC5_UNORM,              8 },
	{ rhi::Format::BC5_SNORM,            DXGI_FORMAT_BC5_SNORM,              8 },
	{ rhi::Format::BC6H_UFLOAT,          DXGI_FORMAT_BC6H_UF16,              8 },
	{ rhi::Format::BC6H_SFLOAT,          DXGI_FORMAT_BC6H_SF16,              8 },
	{ rhi::Format::BC7_UNORM,            DXGI_FORMAT_BC7_UNORM,              8 },
	{ rhi::Format::BC7_UNORM_SRGB,       DXGI_FORMAT_BC7_UNORM_SRGB,         8 },
	};
	ComPtr<IDStorageCompressionCodec> g_bufferCompression;

	template<typename T>
	static std::remove_reference_t<T> Compress(arc::Compression compression, T&& source)
	{
		if (compression == marc::Compression::None)
		{
			return source;
		}
		else
		{
			size_t maxSize;
			if (compression == marc::Compression::GDeflate)
				maxSize = g_bufferCompression->CompressBufferBound(static_cast<uint32_t>(source.size()));
			else if (compression == marc::Compression::Zlib)
				maxSize = static_cast<size_t>(compressBound(static_cast<uLong>(source.size())));
			else
				throw std::runtime_error("Unknown Compression type");

			std::remove_reference_t<T> dest;
			dest.resize(maxSize);

			size_t actualCompressedSize = 0;

			HRESULT compressionResult = S_OK;

			if (compression == marc::Compression::GDeflate)
			{
				compressionResult = g_bufferCompression->CompressBuffer(
					reinterpret_cast<const void*>(source.data()),
					static_cast<uint32_t>(source.size()),
					DSTORAGE_COMPRESSION_BEST_RATIO,
					reinterpret_cast<void*>(dest.data()),
					static_cast<uint32_t>(dest.size()),
					&actualCompressedSize);
			}
			else if (compression == marc::Compression::Zlib)
			{
				throw std::runtime_error("Zlib is not supported");
			}

			if (FAILED(compressionResult))
			{
				std::cout << "Failed to compress data using CompressBuffer, hr = 0x" << std::hex << compressionResult
					<< std::endl;
				std::abort();
			}

			dest.resize(actualCompressedSize);

			return dest;
		}
	}

	std::string Compress(arc::Compression compression, std::stringstream sourceStream)
	{
		auto source = sourceStream.str();
		return Compress(compression, std::move(source));
	}

	void Set(float dest[4], Sphere const& src)
	{
		dest[0] = src.Centre.x;
		dest[1] = src.Centre.y;
		dest[2] = src.Centre.z;
		dest[3] = src.Radius;
	}

	void Set(float dest[3], DirectX::XMFLOAT3 const& src)
	{
		dest[0] = src.x;
		dest[1] = src.y;
		dest[2] = src.z;
	}

	class Exporter
	{
	public:
		static void Export(
			std::ostream& out,
			Compression compression,
			TexConversionFlags extraTextureFlags,
			uint32_t stagingBufferSizeBytes,
			std::filesystem::path rootPath,
			ModelData const& modelData)
		{
			Exporter exporter(out, compression, extraTextureFlags, stagingBufferSizeBytes, rootPath, modelData);
			exporter.Export();
		}

	public:
		Exporter(
			std::ostream& out,
			Compression compression,
			TexConversionFlags extraTextureFlags,
			uint32_t stagingBufferSizeBytes,
			std::filesystem::path rootPath,
			ModelData const& modelData)
			: m_out(out)
			, m_compression(compression)
			, m_extraTextureFlags(extraTextureFlags)
			, m_rootPath(rootPath)
			, m_modelData(modelData)
		{
			if (auto hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)); FAILED(hr))
			{
				PHX_ERROR("Failed to create D3D12 device");
				throw std::runtime_error("Failed to create D3D12 device");
			}
		}

	private:
		void Export()
		{
			// -- Construct layout ---
			BinaryBuilder fileBuilder;
			size_t headerOffset = fileBuilder.Reserve<Header>();

			this->WriteTextures();

			BinaryBuilder gpuDataBuilder;
			this->WriteUnstructuredGpuData(gpuDataBuilder);
			size_t gpuDataOffset = fileBuilder.Reserve(gpuDataBuilder);

			BinaryBuilder cpuMetadataBuilder;
			this->WriteCpuMetadata(cpuMetadataBuilder);
			size_t cpuMetadataOffset = fileBuilder.Reserve(cpuMetadataBuilder);

			BinaryBuilder cpuDataBuilder;
			this->WriteCpuData(cpuDataBuilder);
			size_t cpuDataOffset = fileBuilder.Reserve(cpuDataBuilder);


			fileBuilder.Commit();

			// -- Fill data ---
			Header* header = fileBuilder.Place<Header>(headerOffset);
			header->Id = arc::Id;
			header->Version = CURRENT_PARC_FILE_VERSION;
			Set(header->BoundingSphere, m_modelData.BoundingSphere);
			Set(header->MinPos, this->m_modelData.BoundingBox.Min);
			Set(header->MaxPos, this->m_modelData.BoundingBox.Max);

			fileBuilder.Place(gpuDataOffset, gpuDataBuilder);
			fileBuilder.Place(cpuMetadataOffset, cpuMetadataBuilder);
			fileBuilder.Place(cpuDataOffset, cpuDataBuilder);

			this->m_out.write((char*)fileBuilder.Data(), fileBuilder.Size());
		}


	private:
		void WriteTextures()
		{
			for (size_t i = 0; i < m_modelData.TextureNames.size(); ++i)
			{
				std::string const& textureName = m_modelData.TextureNames[i];
				uint8_t flags = this->m_modelData.TextureOptions[i];
				flags |= m_extraTextureFlags;
				this->WriteTexture(textureName, flags);
			}
		}

		void WriteTexture(std::string const& name, uint8_t flags)
		{
			std::filesystem::path texturePath = this->m_rootPath;
			texturePath /= name;
			texturePath = absolute(texturePath);

			auto image = TextureCompiler::BuildDDS(texturePath.string().c_str(), flags);
			if (!image)
			{
				throw std::runtime_error("Texture load failed");
			}

			DirectX::TexMetadata const& metadata = image->GetMetadata();
			std::vector<D3D12_SUBRESOURCE_DATA> subresources; 
			auto hr = DirectX::PrepareUpload(
				m_device.Get(),
				image->GetImages(),
				image->GetImageCount(),
				image->GetMetadata(),
				subresources);
			if (FAILED(hr))
			{
				PHX_ERROR("'%s' failed to prepare layout ", texturePath.c_str());
				throw std::runtime_error("Texture preparation failed");
			}

			D3D12_RESOURCE_DESC desc{};
			desc.Width = static_cast<UINT>(metadata.width);
			desc.Height = static_cast<UINT>(metadata.height);
			desc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
			desc.DepthOrArraySize = (metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE3D)
				? static_cast<UINT16>(metadata.depth)
				: static_cast<UINT16>(metadata.arraySize);
			desc.Format = metadata.format;
			desc.SampleDesc.Count = 1;
			desc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);

			auto const totalSubresourceCount = CD3DX12_RESOURCE_DESC(desc).Subresources(m_device.Get());

			std::vector<GpuRegion> regions;

			std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(totalSubresourceCount);
			std::vector<UINT> numRows(totalSubresourceCount);
			std::vector<UINT64> rowSizes(totalSubresourceCount);

			uint64_t totalBytes = 0;

			uint32_t currentSubresource = 0;

			this->m_device->GetCopyableFootprints(
				&desc,
				currentSubresource,
				totalSubresourceCount - currentSubresource,
				0,
				layouts.data(),
				numRows.data(),
				rowSizes.data(),
				&totalBytes);

			while (totalBytes > m_stagingBufferSizeBytes)
			{
				m_device->GetCopyableFootprints(
					&desc,
					currentSubresource,
					1,
					0,
					layouts.data(),
					numRows.data(),
					rowSizes.data(),
					&totalBytes);

				if (totalBytes > m_stagingBufferSizeBytes)
				{
					PHX_ERROR("Mip %d won't fit in the staging buffer. Try adding more space to the input config", currentSubresource);
					std::exit(1);
				}

				std::stringstream regionName;
				regionName << name << " mip " << currentSubresource;

				regions.push_back(WriteTextureRegion(
					currentSubresource,
					1,
					layouts,
					numRows,
					rowSizes,
					totalBytes,
					subresources,
					regionName.str()));

				++currentSubresource;

				if (currentSubresource < totalSubresourceCount)
				{
					m_device->GetCopyableFootprints(
						&desc,
						currentSubresource,
						totalSubresourceCount - currentSubresource,
						0,
						layouts.data(),
						numRows.data(),
						rowSizes.data(),
						&totalBytes);
				}
			}

			GpuRegion remainingMipsRegion{};

			if (currentSubresource < totalSubresourceCount)
			{
				std::stringstream regionName;
				regionName << name << " mips " << currentSubresource << " to " << totalSubresourceCount;
				remainingMipsRegion = WriteTextureRegion(
					currentSubresource,
					totalSubresourceCount - currentSubresource,
					layouts,
					numRows,
					rowSizes,
					totalBytes,
					subresources,
					regionName.str());
			}

			TextureMetadata textureMetadata;
			textureMetadata.SingleMips = std::move(regions);
			textureMetadata.RemainingMips = remainingMipsRegion;

			m_textureMetadata.push_back(std::move(textureMetadata));

			m_textureDescs.push_back(desc);
		}

		void WriteUnstructuredGpuData(BinaryBuilder& builder);
		void WriteCpuMetadata(BinaryBuilder& builder);
		void WriteCpuData(BinaryBuilder& builder);


		arc::GpuRegion WriteTextureRegion(
			uint32_t currentSubresource,
			uint32_t numSubresources,
			std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> const& layouts,
			std::vector<UINT> const& numRows,
			std::vector<UINT64> const& rowSizes,
			uint64_t totalBytes,
			std::vector<D3D12_SUBRESOURCE_DATA> const& subresources,
			std::string const& name)
		{
			std::vector<char> data(totalBytes);

			for (auto i = 0u; i < numSubresources; ++i)
			{
				auto const& layout = layouts[i];
				auto const& subresource = subresources[currentSubresource + i];

				D3D12_MEMCPY_DEST memcpyDest{};
				memcpyDest.pData = data.data() + layout.Offset;
				memcpyDest.RowPitch = layout.Footprint.RowPitch;
				memcpyDest.SlicePitch = layout.Footprint.RowPitch * numRows[i];

				MemcpySubresource(
					&memcpyDest,
					&subresource,
					static_cast<SIZE_T>(rowSizes[i]),
					numRows[i],
					layout.Footprint.Depth);
			}

			return WriteRegion<void>(data, name.c_str());
		}
	private:
		template<typename T, typename C>
		Region<T> WriteRegion(C uncompressedRegion, char const* name)
		{
			size_t uncompressedSize = uncompressedRegion.size();

			C compressedRegion;

			Compression compression = m_compression;

			if (compression == Compression::None)
			{
				compressedRegion = std::move(uncompressedRegion);
			}
			else
			{
				compressedRegion = Compress(m_compression, uncompressedRegion);
				if (compressedRegion.size() > uncompressedSize)
				{
					compression = Compression::None;
					compressedRegion = std::move(uncompressedRegion);
				}
			}

			Region<T> r;
			r.Compression = compression;
			r.Data.Offset = static_cast<uint32_t>(m_out.tellp());
			r.CompressedSize = static_cast<uint32_t>(compressedRegion.size());
			r.UncompressedSize = static_cast<uint32_t>(uncompressedSize);

			if (r.Compression == Compression::None)
			{
				assert(r.CompressedSize == r.UncompressedSize);
			}

			m_out.write(compressedRegion.data(), compressedRegion.size());

			auto toString = [](Compression c)
				{
					switch (c)
					{
					case Compression::None:
						return "Uncompressed";
					case Compression::GDeflate:
						return "GDeflate";
					case Compression::Zlib:
						return "Zlib";
					default:
						throw std::runtime_error("Unknown compression format");
					}
				};

			std::cout << r.Data.Offset << ":  " << name << " " << toString(r.Compression) << " " << r.UncompressedSize
				<< " --> " << r.CompressedSize << "\n";

			return r;
		}
	private:
		struct TextureMetadata
		{
			std::vector<GpuRegion> SingleMips;
			GpuRegion RemainingMips;
		};

		std::vector<TextureMetadata> m_textureMetadata;
		std::vector<D3D12_RESOURCE_DESC> m_textureDescs;
		ComPtr<ID3D12Device> m_device;
		std::ostream& m_out;
		Compression m_compression;
		TexConversionFlags m_extraTextureFlags;
		uint32_t m_stagingBufferSizeBytes;
		std::filesystem::path m_rootPath;
		const ModelData& m_modelData;
	};
}

// "{ \"input\" : \"C:\\Users\\dipao\\source\\repos\\Impulse21\\Phoenix-Engine\\Assets\\Main.1_Sponza\\NewSponza_Main_glTF_002.gltf\", \"output_file\": \"Sponza.phxarc", \"compression\" : \"GDeflate\" }"
// "{ \"input\" : \"..\\..\\..\\Assets\\Box.gltf\", \"output_dir\": \"..\\..\\..\\Output\\resources\",  \"compression\" : \"None\" }"
int main(int argc, const char** argv)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	assert(SUCCEEDED(hr));

	Log::Initialize();
	if (argc == 0)
	{
		PHX_INFO("Input json is expected");
		return -1;
	}

	json inputSettings = json::parse(argv[1]);

	const std::string inputTag = "input";
	const std::string outputTag = "output_file";
	const std::string compressionTag = "compression";
	if (!inputSettings.contains(inputTag))
	{
		PHX_ERROR("Input is required");
		return -1;
	}

	if (!inputSettings.contains(outputTag))
	{
		PHX_ERROR("Missing output file tag");
		return -1;
	}

	const std::string& gltfInput = inputSettings[inputTag];
	const std::string& outputFilename = inputSettings[outputTag];

	bool useGDeflate = false;
	if (inputSettings.contains(compressionTag))
	{
		const std::string& compressionStr = inputSettings[compressionTag];
		useGDeflate = compressionStr == "gdeflate";
	}

	PHX_INFO("Creating PhxArchive '%s' from '%s'", outputFilename.c_str(), gltfInput.c_str());
	std::filesystem::path gltfInputPath(gltfInput);
	gltfInputPath.make_preferred();
	std::unique_ptr<IFileSystem> fs = FileSystemFactory::CreateRelativeFileSystem(FileSystemFactory::CreateNativeFileSystem(), gltfInputPath.parent_path());
	

	phxModelImporterGltf gltfImporter(fs.get());
	// Import Model from GLTF
	phx::StopWatch elapsedTime;

	ModelData model = {};
	gltfImporter.Import(gltfInput, model);
	PHX_INFO("Importing GLTF File took %f seconds", elapsedTime.Elapsed().GetSeconds());

	if (useGDeflate)
	{
        // Get the buffer compression interface for DSTORAGE_COMPRESSION_FORMAT_GDEFLATE
		constexpr uint32_t NumCompressionThreads = 6;
		SUCCEEDED(
			DStorageCreateCompressionCodec(
				DSTORAGE_COMPRESSION_FORMAT_GDEFLATE,
				NumCompressionThreads,
				IID_PPV_ARGS(&g_bufferCompression)));
	}

	Compression compression = Compression::None;
	if (useGDeflate)
		compression = Compression::GDeflate;


	bool useBC = true;
	TexConversionFlags extraTextureFlags{};
	if (useBC)
	{
		extraTextureFlags = static_cast<TexConversionFlags>(extraTextureFlags | kDefaultBC);
	}

	uint32_t stagingBufferSize = 256_MiB;
	std::filesystem::path outputPath(outputFilename);
	outputPath.make_preferred();

	std::ofstream outStream(outputPath, std::ios::out | std::ios::trunc | std::ios::binary);
	elapsedTime.Begin();
	Exporter::Export(outStream, compression, extraTextureFlags, stagingBufferSize, gltfInputPath.parent_path(), model);
	PHX_INFO("Exporting Archive file '%s' took %f seconds", outputFilename, elapsedTime.Elapsed().GetSeconds());

}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
