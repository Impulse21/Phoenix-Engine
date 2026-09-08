#include "MeshCompiler.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Resources/Intermediate/IntermediateMesh.h>

#include <PhxEngine/Renderer/Shaders/Interop.h>

#include <PhxEngine/Core/BinaryBuilder.h>

using namespace phx;

namespace
{
    template<class TOffsetHandle, class TStorageType>
    void Reserve(size_t num_elements, BinaryBuilder<TOffsetHandle>& vertex_builder, renderer::VertexStreamDesc& stream_desc)
    {
        if (num_elements == 0)
            return;

        const size_t stride = sizeof(TStorageType);
        const size_t stream_size = stride * num_elements;
        const OffsetHandle offset = vertex_builder.Reserve(stream_size, 16u);

        stream_desc.SetOffset(offset);
        stream_desc.SetStride(stride);
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

compiler::Mesh compiler::CompileMesh(const IntermediateMesh& mesh)
{
    BinaryBuilder bin_builder;
    
    Mesh compiled_mesh;
    compiled_mesh.primitives.resize(mesh.primitives.size());

    BinaryBuilder vertex_builder;
    BinaryBuilder index_builder;

    std::vector<renderer::VertexStreamsHeader> vertex_headers(mesh.primitives.size());
    for (size_t i = 0; i < mesh.primitives.size(); ++i)
    {
        const IntermediateMesh::Primitive& prim = mesh.primitives[i];
        compiler::Mesh::PrimitiveView& prim_view = compiled_mesh.primitives[i];

        // compiled_mesh.bbox_ls.AddBoundingBox(prim.bbox_ls);
        // compiled_mesh.bounds_ls.Union(prim.bounds_ls);

        // TOOD: Need to sort this out.
        // prim_view.material_id = prim.material_id;
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
        prim_view.index_count = prim.indices.size();
        prim_view.index_offset = index_builder.ReserveArray<uint32_t>(prim_view.index_count, sizeof(uint32_t));
    }

    vertex_builder.Commit();
    index_builder.Commit();
    for (size_t i = 0; i < mesh.primitives.size(); ++i)
    {
        const IntermediateMesh::Primitive& prim = mesh.primitives[i];
        const compiler::Mesh::PrimitiveView& prim_view = compiled_mesh.primitives[i];
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