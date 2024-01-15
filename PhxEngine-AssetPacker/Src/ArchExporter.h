#pragma once

#include <fstream>
#include <PhxEngine/Assets/PhxArchFormat.h>
#include <iostream>
#include <assert.h>

class IArchExporter
{
public:
	virtual ~IArchExporter() = default;

	virtual void Export() = 0;
};

struct cgltf_data;

class ArchGltfExporter : public IArchExporter
{
public:
	ArchGltfExporter(
		std::ostream& stream,
		cgltf_data* objects,
		PhxEngine::Assets::PhxArch::Compression compression = PhxEngine::Assets::PhxArch::Compression::None);

	void Export() override;

private:
	PhxEngine::Assets::PhxArch::Region<PhxEngine::Assets::PhxArch::CpuMetadataHeader> WriteCpuMetadata();

    template<typename T, typename C>
    PhxEngine::Assets::PhxArch::Region<T> WriteRegion(C uncompressedRegion, char const* name)
    {
        using namespace  PhxEngine::Assets;

        size_t uncompressedSize = uncompressedRegion.size();
        C compressedRegion;
        PhxEngine::Assets::PhxArch::Compression compression = this->m_compression;

        if (compression == PhxArch::Compression::None)
        {
            compressedRegion = std::move(uncompressedRegion);
        }
        else
        {
            assert(false);
#if false
            compressedRegion = Compress(m_compression, uncompressedRegion);
            if (compressedRegion.size() > uncompressedSize)
            {
                compression = Compression::None;
                compressedRegion = std::move(uncompressedRegion);
            }
#endif
        }

        PhxEngine::Assets::PhxArch::Region<T> r;
        r.Compression = compression;
        r.Data.Offset = static_cast<uint32_t>(this->m_stream.tellp());
        r.CompressedSize = static_cast<uint32_t>(compressedRegion.size());
        r.UncompressedSize = static_cast<uint32_t>(uncompressedSize);

        if (r.Compression == PhxArch::Compression::None)
        {
            assert(r.CompressedSize == r.UncompressedSize);
        }

        this->m_stream.write(compressedRegion.data(), compressedRegion.size());

        auto toString = [](PhxArch::Compression c)
            {
                switch (c)
                {
                case PhxArch::Compression::None:
                    return "Uncompressed";
                case PhxArch::Compression::GDeflate:
                    return "GDeflate";
                default:
                    throw std::runtime_error("Unknown compression format");
                }
            };

        std::cout << r.Data.Offset << ":  " << name << " " << toString(r.Compression) << " " << r.UncompressedSize
            << " --> " << r.CompressedSize << "\n";

        return r;
    }


private:
	std::ostream& m_stream;
	cgltf_data* m_gltfData;
	PhxEngine::Assets::PhxArch::Compression m_compression;
};

