#include "ArchExporter.h"

#include <cgltf.h>

#include <sstream>
#include <vector>
#include <assert.h>


using namespace PhxEngine;
using namespace PhxEngine::Assets;

namespace
{
    void Patch(std::ostream& s, std::streampos pos, void const* data, size_t size)
    {
        auto oldPos = s.tellp();
        s.seekp(pos);
        s.write(reinterpret_cast<char const*>(data), size);
        s.seekp(oldPos);
    }

    template<typename T>
    void Patch(std::ostream& s, std::streampos pos, T const& value)
    {
        Patch(s, pos, &value, sizeof(value));
    }

    template<typename T>
    void Patch(std::ostream& s, std::streampos pos, std::vector<T> const& values)
    {
        Patch(s, pos, values.data(), values.size() * sizeof(T));
    }

    template<typename T>
    class Fixup
    {
    public:
        Fixup() = default;

        Fixup(std::streampos fixupPos)
            : m_pos(fixupPos)
        {
        }

        void Set(std::ostream& stream, T const& value) const
        {
            Patch(stream, m_pos, value);
        }

        // Overload that only works when T is a marc::Ptr<> (SFINAE)
        void Set(std::ostream& stream, std::streampos value) const
        {
            T t;
            t.OffsetFromRegionStart = static_cast<uint64_t>(value);
            Set(stream, t);
        }
    private:
        std::streampos m_pos;
    };

    template<typename T, typename FIXUP>
    Fixup<FIXUP> MakeFixup(std::streampos startPos, T const* src, FIXUP const* fixup)
    {
        auto byteSrc = reinterpret_cast<uint8_t const*>(src);
        auto byteField = reinterpret_cast<uint8_t const*>(fixup);
        std::ptrdiff_t offset = byteField - byteSrc;

        if ((offset + sizeof(*fixup)) > sizeof(*src))
        {
            throw std::runtime_error("Fixup outside of src structure");
        }

        std::streampos fixupPos = startPos + offset;

        return Fixup<FIXUP>(fixupPos);
    }

	template <typename T, typename... FIXUPS>
    std::tuple<Fixup<FIXUPS>...> WriteStruct(std::ostream& out, T const* src, FIXUPS const*... fixups)
    {
        auto startPos = out.tellp();
        out.write(reinterpret_cast<char const*>(src), sizeof(*src));

        return std::make_tuple(MakeFixup(startPos, src, fixups)...);
    }

    template<typename T>
    void WriteArray(std::ostream& s, T const* data, size_t count)
    {
        s.write((char*)data, sizeof(*data) * count);
    }

    template<typename CONTAINER>
    PhxArch::Array<typename CONTAINER::value_type> WriteArray(std::ostream& s, CONTAINER const& data)
    {
        auto pos = s.tellp();
        WriteArray(s, data.data(), data.size());

        PhxArch::Array<typename CONTAINER::value_type> array;
        array.Data.Offset = static_cast<uint32_t>(pos);

        return array;
    }
}
ArchGltfExporter::ArchGltfExporter(std::ostream& stream, cgltf_data* objects, PhxArch::Compression compression)
	: m_stream(stream)
	, m_compression(compression)
	, m_gltfData(objects)
{
}

void ArchGltfExporter::Export()
{
	PhxArch::Header header;
	header.Id[0] = 'P';
	header.Id[1] = 'P';
	header.Id[2] = 'K';
	header.Id[3] = 'T';

	header.Version = PhxArch::kCurrentFileFormat;
    auto [fixupHeader] = WriteStruct(this->m_stream, &header, &header);

    header.CpuMetadata = WriteCpuMetadata();

    fixupHeader.Set(this->m_stream, header);
}

PhxArch::Region<PhxArch::CpuMetadataHeader> ArchGltfExporter::WriteCpuMetadata()
{
    std::stringstream strStream;
    PhxArch::CpuMetadataHeader header = {};

    // Write placeholder header; we'll fix this up later when we've
    // written everything else out
    auto [fixupHeader] = WriteStruct(strStream, &header, &header);

    header.NumPackedAssets = 
        this->m_gltfData->meshes_count + 
        this->m_gltfData->materials_count + 
        this->m_gltfData->textures_count;

    std::vector<PhxArch::AssetMetadata> assetsMetadata;
    assetsMetadata.reserve(header.NumPackedAssets);

    // Write out meshes
    for (int i = 0; i < this->m_gltfData->meshes_count; i++)
    {
        cgltf_mesh& gltfmesh = this->m_gltfData->meshes[i];

        PhxArch::AssetMetadata metadata;
        metadata.Id[0] = 'P';
        metadata.Id[1] = 'M';
        metadata.Id[2] = 'S';
        metadata.Id[3] = 'H';

        // Get Name from registery

        metadata.Name = WriteArray(strStream, std::string(gltfmesh.name)).Data;
        strStream.put(0); // null terminate the name string

        // Collect Dependancies
        // metadata.Dependencies = WriteArray(strStream, std::string("")).Data;
        assetsMetadata.push_back(metadata);
    }

    // Write out Mtls
    for (int i = 0; i < this->m_gltfData->materials_count; i++)
    {
        cgltf_material& gltfMtl = this->m_gltfData->materials[i];

        PhxArch::AssetMetadata metadata;
        metadata.Id[0] = 'P';
        metadata.Id[1] = 'M';
        metadata.Id[2] = 'A';
        metadata.Id[3] = 'T';

        // Get Name from registery

        metadata.Name = WriteArray(strStream, std::string(gltfMtl.name)).Data;
        strStream.put(0); // null terminate the name string
        // Collect Dependancies
        // Set Textures
        // metadata.Dependencies = WriteArray(strStream, std::string("")).Data;

        assetsMetadata.push_back(metadata);
    }

    // Write out Mtls
    for (int i = 0; i < this->m_gltfData->textures_count; i++)
    {
        cgltf_texture& gltfTexture = this->m_gltfData->textures[i];

        PhxArch::AssetMetadata metadata;
        metadata.Id[0] = 'P';
        metadata.Id[1] = 'T';
        metadata.Id[2] = 'E';
        metadata.Id[3] = 'X';

        // Get Name from registery

        metadata.Name = WriteArray(strStream, std::string(gltfTexture.name)).Data;
        strStream.put(0); // null terminate the name string

        // Collect Dependancies
        // Set Textures
        // metadata.Dependencies = WriteArray(strStream, std::string("")).Data;
        assetsMetadata.push_back(metadata);
    }

    header.AssetMetadata = WriteArray(strStream, assetsMetadata);
    // Fixup the CPU data header
    fixupHeader.Set(strStream, header);

    return this->WriteRegion<PhxArch::CpuMetadataHeader>(strStream.str(), "CPU Metadata");
}
