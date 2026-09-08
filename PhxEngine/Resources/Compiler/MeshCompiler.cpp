#include "MeshCompiler.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Resources/Intermediate/IntermediateMesh.h>
#include <PhxEngine/Resources/MeshFileFormat.h>

#include <PhxEngine/Renderer/Shaders/Interop.h>

#include <PhxEngine/Core/BinaryBuilder.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/VFS/VFS.h>

#include <cstring>

using namespace phx;
using namespace phx::resources;

namespace
{
    constexpr Log::Channel k_log = { "MeshCompiler" };

    // FNV-1a 64-bit over raw bytes -- matches VFS.cpp's HashPath style, just
    // widened and applied to binary content instead of a NUL-terminated
    // string. Used only for the .phxmsh header's staleness-check hash.
    u64 HashBytes(const std::byte* data, size_t size)
    {
        u64 hash = 14695981039346656037ull;
        for (size_t i = 0; i < size; ++i)
        {
            hash ^= static_cast<u64>(data[i]);
            hash *= 1099511628211ull;
        }
        return hash;
    }

    u32 AlignUp(u32 value, u32 alignment)
    {
        return (value + (alignment - 1)) & ~(alignment - 1);
    }

    template<class TOffsetHandle, class TStorageType>
    void Reserve(size_t num_elements, BinaryBuilder<TOffsetHandle>& vertex_builder, renderer::VertexStreamDesc& stream_desc)
    {
        if (num_elements == 0)
            return;

        const size_t stride = sizeof(TStorageType);
        const size_t stream_size = stride * num_elements;
        const TOffsetHandle offset = vertex_builder.Reserve(stream_size, 16u);

        stream_desc.Set(static_cast<uint>(stride), static_cast<uint>(offset));
    }

    template<class TOffsetHandle, class T>
    void Place(Span<T> vertex_data, BinaryBuilder<TOffsetHandle>& vertex_builder, renderer::VertexStreamDesc const& stream_desc)
    {
        if (vertex_data.IsEmpty())
            return;

        auto* dest = vertex_builder.template PlaceType<std::byte>(static_cast<TOffsetHandle>(stream_desc.GetOffset()));

        const size_t stride = stream_desc.GetStride();
        for (size_t v = 0; v < vertex_data.size(); ++v)
        {
            std::memcpy(dest + (v * stride), &vertex_data[v], stride);
        }
    }
}

CompiledMesh phx::resources::CompileMesh(const IntermediateMesh& mesh)
{
    CompiledMesh compiled_mesh;
    compiled_mesh.primitives.resize(mesh.primitives.size());

    BinaryBuilder vertex_builder;
    BinaryBuilder index_builder;

    std::vector<renderer::VertexStreamsHeader> vertex_headers(mesh.primitives.size());
    for (size_t i = 0; i < mesh.primitives.size(); ++i)
    {
        const IntermediateMesh::Primitive& prim = mesh.primitives[i];
        CompiledMesh::PrimitiveView& prim_view = compiled_mesh.primitives[i];

        // compiled_mesh.bbox_ls.AddBoundingBox(prim.bbox_ls);
        // compiled_mesh.bounds_ls.Union(prim.bounds_ls);

        prim_view.material_name = prim.material_name;
        prim_view.vertex_offset = vertex_builder.Reserve<renderer::VertexStreamsHeader>();

        renderer::VertexStreamsHeader& header = vertex_headers[i];
        PHX_ASSERT(!prim.positions.empty() && "Submesh must have at least position vertex stream.");

        Reserve<OffsetHandle, hlslpp::interop::float3>(prim.positions.size(), vertex_builder, header.desc[renderer::VertexStream_Position]);
        
        if (!prim.normals.empty())
            Reserve<OffsetHandle, hlslpp::interop::float3>(prim.normals.size(), vertex_builder, header.desc[renderer::VertexStream_Normal]);
        
        if (!prim.texCoords_0.empty())
            Reserve<OffsetHandle, hlslpp::interop::float2>(prim.texCoords_0.size(), vertex_builder, header.desc[renderer::VertexStream_Texcoord0]);
        
        if (!prim.texCoords_1.empty())
            Reserve<OffsetHandle, hlslpp::interop::float2>(prim.texCoords_1.size(), vertex_builder, header.desc[renderer::VertexStream_Texcoord1]);
        
        if (!prim.tangents.empty())
            Reserve<OffsetHandle, hlslpp::interop::float4>(prim.tangents.size(), vertex_builder, header.desc[renderer::VertexStream_Tangent]);
        
        if (!prim.colour.empty())
            Reserve<OffsetHandle, hlslpp::interop::float3>(prim.colour.size(), vertex_builder, header.desc[renderer::VertexStream_Colour0]);
        
        if (!prim.joints_0.empty())
            Reserve<OffsetHandle, hlslpp::interop::uint4>(prim.joints_0.size(), vertex_builder, header.desc[renderer::VertexStream_Joints0]);
        
        if (!prim.weights_0.empty())
            Reserve<OffsetHandle, hlslpp::interop::float4>(prim.weights_0.size(), vertex_builder, header.desc[renderer::VertexStream_Weights0]);

        PHX_ASSERT(!prim.indices.empty() && "Submesh must have an index buffer.");
        PHX_ASSERT(prim.indices.size() <= UINT32_MAX && "Index count exceeds 32 bits.");
        prim_view.index_count = static_cast<uint32_t>(prim.indices.size());
        prim_view.index_offset = index_builder.ReserveArray<uint32_t>(prim_view.index_count, sizeof(uint32_t));
    }

    vertex_builder.Commit();
    index_builder.Commit();
    for (size_t i = 0; i < mesh.primitives.size(); ++i)
    {
        const IntermediateMesh::Primitive& prim = mesh.primitives[i];
        const CompiledMesh::PrimitiveView& prim_view = compiled_mesh.primitives[i];
        const renderer::VertexStreamsHeader& header = vertex_headers[i];

        auto* header_dest = vertex_builder.PlaceType<renderer::VertexStreamsHeader>(prim_view.vertex_offset);
        std::memcpy(header_dest, &header, sizeof(renderer::VertexStreamsHeader));

        Place<OffsetHandle, hlslpp::float3>(prim.positions, vertex_builder, header.desc[renderer::VertexStream_Position]);
        
        if (!prim.normals.empty())
            Place<OffsetHandle, hlslpp::float3>(prim.normals, vertex_builder, header.desc[renderer::VertexStream_Normal]);

        if (!prim.texCoords_0.empty())
            Place<OffsetHandle, hlslpp::float2>(prim.texCoords_0, vertex_builder, header.desc[renderer::VertexStream_Texcoord0]);

        if (!prim.texCoords_1.empty())
            Place<OffsetHandle, hlslpp::float2>(prim.texCoords_1, vertex_builder, header.desc[renderer::VertexStream_Texcoord1]);

        if (!prim.tangents.empty())
            Place<OffsetHandle, hlslpp::float4>(prim.tangents, vertex_builder, header.desc[renderer::VertexStream_Tangent]);

        if (!prim.colour.empty())
            Place<OffsetHandle, hlslpp::float3>(prim.colour, vertex_builder, header.desc[renderer::VertexStream_Colour0]);

        if (!prim.joints_0.empty())
            Place<OffsetHandle, hlslpp::uint4>(prim.joints_0, vertex_builder, header.desc[renderer::VertexStream_Joints0]);

        if (!prim.weights_0.empty())
            Place<OffsetHandle, hlslpp::float4>(prim.weights_0, vertex_builder, header.desc[renderer::VertexStream_Weights0]);

        void* dest_index = index_builder.Place(prim_view.index_offset);
        std::memcpy(dest_index, prim.indices.data(), sizeof(uint32_t) * prim_view.index_count);
    }

    compiled_mesh.vertex_buffer = vertex_builder.Finalize();
    compiled_mesh.index_buffer = index_builder.Finalize();

    return compiled_mesh;
}

MemoryBuffer phx::resources::SerializeMesh(const CompiledMesh& mesh, const std::string& source_path, const std::string& mesh_name)
{
    // -- string table -- offset 0 is a sentinel empty string ("no material" / absent path)
    std::string string_table;
    string_table.push_back('\0');

    auto add_string = [&string_table](const std::string& s) -> u32
    {
        if (s.empty())
            return 0;

        const u32 offset = static_cast<u32>(string_table.size());
        string_table.append(s);
        string_table.push_back('\0');
        return offset;
    };

    const u32 source_path_offset = add_string(source_path);

    std::vector<u32> material_name_offsets(mesh.primitives.size());
    for (size_t i = 0; i < mesh.primitives.size(); ++i)
        material_name_offsets[i] = add_string(mesh.primitives[i].material_name);

    // -- provenance hash: re-read the source file's bytes --
    u64 source_content_hash = 0;
    {
        MemoryBuffer source_bytes = VFS::ReadFile(source_path.c_str());
        if (!source_bytes.IsEmpty())
            source_content_hash = HashBytes(source_bytes.Data(), source_bytes.Size());
        else
            PHX_LOG_WARN(k_log, "Could not re-read source '{0}' (mesh '{1}') to compute content hash", source_path, mesh_name);
    }

    // -- GPU chunk: vertex blob then index blob, 16-byte aligned boundary --
    const u32 vertex_bytes         = static_cast<u32>(mesh.vertex_buffer.Size());
    const u32 index_section_offset = AlignUp(vertex_bytes, 16u);
    const u32 index_bytes          = static_cast<u32>(mesh.index_buffer.Size());
    const u32 gpu_chunk_size       = index_section_offset + index_bytes;

    // -- CPU chunk: CpuData + DrawInfo[] --
    BinaryBuilder<u32> cpu_builder;
    const u32 cpu_data_offset  = cpu_builder.Reserve<MeshResource::CpuData>();
    const u32 draw_info_offset = cpu_builder.ReserveArray<MeshResource::CpuData::DrawInfo>(mesh.primitives.size());
    cpu_builder.Commit();

    auto* cpu_data  = cpu_builder.PlaceType<MeshResource::CpuData>(cpu_data_offset);
    auto* draw_info = cpu_builder.PlaceType<MeshResource::CpuData::DrawInfo>(draw_info_offset);

    cpu_data->draw_info.Set(draw_info);
    cpu_data->draw_info_count = static_cast<u32>(mesh.primitives.size());

    for (size_t i = 0; i < mesh.primitives.size(); ++i)
    {
        const CompiledMesh::PrimitiveView& prim_view = mesh.primitives[i];
        draw_info[i].index_count              = prim_view.index_count;
        draw_info[i].index_byte_offset        = index_section_offset + prim_view.index_offset;
        draw_info[i].stream_header_byte_offset = prim_view.vertex_offset;
        draw_info[i].material_name_offset     = material_name_offsets[i];
    }

    MemoryBuffer cpu_chunk_bytes = cpu_builder.Finalize();

    // -- assemble the file: header, chunk table, CPU chunk, GPU chunk, strings (each 16-byte aligned) --
    const u32 header_size           = static_cast<u32>(sizeof(ResourceFileHeader));
    const u32 chunk_table_size      = static_cast<u32>(sizeof(ChunkEntry) * 2);
    const u32 cpu_chunk_file_offset = AlignUp(header_size + chunk_table_size, 16u);
    const u32 gpu_chunk_file_offset = AlignUp(cpu_chunk_file_offset + static_cast<u32>(cpu_chunk_bytes.Size()), 16u);
    const u32 strings_file_offset   = AlignUp(gpu_chunk_file_offset + gpu_chunk_size, 16u);
    const u32 total_size            = strings_file_offset + static_cast<u32>(string_table.size());

    MemoryBuffer file_bytes(total_size, std::byte{ 0 });

    auto* header = reinterpret_cast<ResourceFileHeader*>(file_bytes.Data());
    header->magic               = kMeshFileMagic;
    header->version              = kMeshFileVersion;
    header->chunk_count          = 2;
    header->source_path_offset   = source_path_offset;
    header->source_content_hash  = source_content_hash;
    header->strings_offset       = strings_file_offset;
    header->strings_size         = static_cast<u32>(string_table.size());

    auto* chunks = reinterpret_cast<ChunkEntry*>(file_bytes.Data() + header_size);
    chunks[0] = ChunkEntry{ kMeshChunkType_Cpu, cpu_chunk_file_offset, static_cast<u32>(cpu_chunk_bytes.Size()) };
    chunks[1] = ChunkEntry{ kMeshChunkType_Gpu, gpu_chunk_file_offset, gpu_chunk_size };

    std::memcpy(file_bytes.Data() + cpu_chunk_file_offset, cpu_chunk_bytes.Data(), cpu_chunk_bytes.Size());

    std::memcpy(file_bytes.Data() + gpu_chunk_file_offset, mesh.vertex_buffer.Data(), vertex_bytes);
    std::memcpy(file_bytes.Data() + gpu_chunk_file_offset + index_section_offset, mesh.index_buffer.Data(), index_bytes);

    std::memcpy(file_bytes.Data() + strings_file_offset, string_table.data(), string_table.size());

    return file_bytes;
}

bool phx::resources::WriteMeshFile(const char* virtual_path, const MemoryBuffer& file_bytes)
{
    return VFS::WriteFile(virtual_path,
        Span<const u8>(reinterpret_cast<const u8*>(file_bytes.Data()), file_bytes.Size()));
}