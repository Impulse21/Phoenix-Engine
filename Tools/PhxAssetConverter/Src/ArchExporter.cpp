#include "ArchExporter.h"

#include <PhxEngine/Core/StringHash.h>

#include <cgltf.h>

#include <sstream>
#include <vector>
#include <assert.h>

#include <PhxEngine/Core/StringHashTable.h>

#include <json.hpp>

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
	header.Id = MAKE_ID('P', 'P', 'K', 'T');
	header.Version = PhxArch::kCurrentFileFormat;

    auto [fixupHeader] = WriteStruct(this->m_stream, &header, &header);

    // CpuAssetMetaData
    header.AssetMetadata = WriteAssetEntries();

    fixupHeader.Set(this->m_stream, header);
}

PhxArch::Region<PhxArch::AssetEntriesHeader> ArchGltfExporter::WriteAssetEntries()
{
    std::stringstream strStream;
    PhxArch::AssetEntriesHeader header = {};

    // Write placeholder header; we'll fix this up later when we've
    // written everything else out
    auto [fixupHeader] = WriteStruct(strStream, &header, &header);

    header.NumEntries =
        this->m_gltfData->meshes_count +
        this->m_gltfData->materials_count +
        this->m_gltfData->textures_count;

    std::vector<PhxArch::AssetEntry> entries;
    entries.reserve(header.NumEntries);

    // Write out meshes
    for (int i = 0; i < this->m_gltfData->meshes_count; i++)
    {
        cgltf_mesh& gltfmesh = this->m_gltfData->meshes[i];

        PhxArch::AssetEntry& entry = entries.emplace_back();
        this->ExportMesh(strStream, entry, gltfmesh);
    }

    // Write out Mtls
    for (int i = 0; i < this->m_gltfData->materials_count; i++)
    {
        cgltf_material& gltfMtl = this->m_gltfData->materials[i];

        PhxArch::AssetEntry& entry = entries.emplace_back();
        this->ExportMtl(strStream, entry, gltfMtl);
    }

    // Write out Mtls
    for (int i = 0; i < this->m_gltfData->textures_count; i++)
    {
        cgltf_texture& gltfTexture = this->m_gltfData->textures[i];
        PhxArch::AssetEntry& entry = entries.emplace_back();
        entry.Id = MAKE_ID('P', 'T', 'E', 'X');
        entry.Name = PhxEngine::Core::StringHash(gltfTexture.name).Value();
        PhxEngine::Core::StringHashTable::RegisterHash(entry.Name, gltfTexture.name);
    }

    header.Entries = WriteArray(strStream, entries);
    strStream.put(0); // null terminate the name string

    fixupHeader.Set(strStream, header);

    return this->WriteRegion<PhxArch::AssetEntriesHeader>(strStream.str(), "Asset Metadata");
}

void ArchGltfExporter::ExportMesh(std::stringstream& s, PhxArch::AssetEntry& entry, cgltf_mesh const& gltfMesh)
{
    entry.Id = MAKE_ID('P', 'M', 'S', 'H');

    entry.Name = PhxEngine::Core::StringHash(gltfMesh.name).Value();
    PhxEngine::Core::StringHashTable::RegisterHash(entry.Name, gltfMesh.name);
}

void ArchGltfExporter::ExportMtl(std::stringstream& s, PhxArch::AssetEntry& entry, cgltf_material const& gltfMtl)
{
    entry.Id = MAKE_ID('P', 'M', 'A', 'T');

    entry.Name = PhxEngine::Core::StringHash(gltfMtl.name).Value();
    PhxEngine::Core::StringHashTable::RegisterHash(entry.Name, gltfMtl.name);

    // Determine dependencies
    std::vector<uint32_t> dependencies;
    dependencies.reserve(4);

    nlohmann::json metadata;
    auto* texture = gltfMtl.pbr_metallic_roughness.base_color_texture.texture;
    if (texture)
    {
        Core::StringHash hash = PhxEngine::Core::StringHash(texture->name);
        dependencies.push_back(hash);
        metadata["albedo_texture"] = hash.Value();
    }

    texture = gltfMtl.pbr_metallic_roughness.metallic_roughness_texture.texture;
    if (texture)
    {
        Core::StringHash hash = PhxEngine::Core::StringHash(texture->name);
        dependencies.push_back(hash);
        metadata["material_texture"] = hash.Value();
    }

    texture = gltfMtl.normal_texture.texture;
    if (texture)
    {
        Core::StringHash hash = PhxEngine::Core::StringHash(texture->name);
        dependencies.push_back(hash);
        metadata["normal_texture"] = hash.Value();
    }

    texture = gltfMtl.emissive_texture.texture;
    if (texture)
    {
        Core::StringHash hash = PhxEngine::Core::StringHash(texture->name);
        dependencies.push_back(hash);
        metadata["emissive_texture"] = hash.Value();
    }

    std::array<float, 4> baseColourV;
    baseColourV[0] = gltfMtl.pbr_metallic_roughness.base_color_factor[0];
    baseColourV[1] = gltfMtl.pbr_metallic_roughness.base_color_factor[1];
    baseColourV[2] = gltfMtl.pbr_metallic_roughness.base_color_factor[2];
    baseColourV[3] = gltfMtl.pbr_metallic_roughness.base_color_factor[3];
    metadata["base_color"] = baseColourV;

    std::array<float, 4> emissiveV;
    emissiveV[0] = gltfMtl.emissive_factor[0];
    emissiveV[1] = gltfMtl.emissive_factor[1];
    emissiveV[2] = gltfMtl.emissive_factor[2];
    emissiveV[3] = gltfMtl.emissive_factor[3];
    metadata["emissive_color"] = emissiveV;

    metadata["metalness"] = gltfMtl.pbr_metallic_roughness.metallic_factor;
    metadata["roughness"] = gltfMtl.pbr_metallic_roughness.roughness_factor;
    metadata["is_double_sided"] = gltfMtl.double_sided;

    entry.Metadata = WriteArray(s, metadata.dump());
}
