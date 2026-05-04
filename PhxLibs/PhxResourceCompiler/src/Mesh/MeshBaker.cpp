#include "PhxResource_pch.h"

#include <PhxResourceCompiler/Mesh/MeshBaker.h>
#include <PhxResourceCompiler/Mesh/MeshTypes.h>

using namespace phx::resource;


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


bool baker::BakeMesh(const RawMesh &raw_mesh, BakedMesh &out_baked_mesh)
{
    out_baked_mesh.sub_meshes.resize(raw_mesh.sub_meshes.size());

    BinaryBuilder vertex_builder;
    BinaryBuilder index_builder;

    std::vector<renderer::VertexStreamsHeader> vertex_headers(raw_mesh.sub_meshes.size());
    for (size_t i = 0; i < raw_mesh.sub_meshes.size(); ++i)
    {
        const IntermediateSubMesh& sub_mesh = raw_mesh.sub_meshes[i];
        BakedMesh::SubMeshView& sub_mesh_view = out_baked_mesh.sub_meshes[i];

        out_baked_mesh.bbox_ls.AddBoundingBox(sub_mesh.bbox_ls);
        out_baked_mesh.bounds_ls.Union(sub_mesh.bounds_ls);

        sub_mesh_view.material_id = sub_mesh.material_id;
        sub_mesh_view.vertex_offset = vertex_builder.Reserve<renderer::VertexStreamsHeader>();

        renderer::VertexStreamsHeader& header = vertex_headers[i];
        Reserve<OffsetHandle, hlslpp::interop::float3>(sub_mesh.positions.size(), vertex_builder, header.Desc[renderer::VertexStream_Position]);
        Reserve<OffsetHandle, hlslpp::interop::float3>(sub_mesh.normals.size(), vertex_builder, header.Desc[renderer::VertexStream_Normal]);
        Reserve<OffsetHandle, hlslpp::interop::float2>(sub_mesh.texCoords_0.size(), vertex_builder, header.Desc[renderer::VertexStream_Texcoord0]);
        Reserve<OffsetHandle, hlslpp::interop::float2>(sub_mesh.texCoords_1.size(), vertex_builder, header.Desc[renderer::VertexStream_Texcoord1]);
        Reserve<OffsetHandle, hlslpp::interop::float4>(sub_mesh.tangents.size(), vertex_builder, header.Desc[renderer::VertexStream_Tangent]);
        Reserve<OffsetHandle, hlslpp::interop::float3>(sub_mesh.colour.size(), vertex_builder, header.Desc[renderer::VertexStream_Colour0]);
        Reserve<OffsetHandle, hlslpp::interop::uint4>(sub_mesh.joints_0.size(), vertex_builder, header.Desc[renderer::VertexStream_Joints0]);
        Reserve<OffsetHandle, hlslpp::interop::float4>(sub_mesh.weights_0.size(), vertex_builder, header.Desc[renderer::VertexStream_Weights0]);
        
        sub_mesh_view.index_count = sub_mesh.indices.size();
        sub_mesh_view.index_offset = index_builder.ReserveArray<uint32_t>(sub_mesh_view.index_count, sizeof(uint32_t));
    }

    vertex_builder.Commit();
    index_builder.Commit();
    for (size_t i = 0; i < raw_mesh.sub_meshes.size(); ++i)
    {
        const IntermediateSubMesh& sub_mesh = raw_mesh.sub_meshes[i];
        const BakedMesh::SubMeshView& sub_mesh_view = out_baked_mesh.sub_meshes[i];
        const renderer::VertexStreamsHeader& header = vertex_headers[i];

        auto* header_dest = vertex_builder.PlaceType<renderer::VertexStreamsHeader>(sub_mesh_view.vertex_offset);
        std::memcpy(header_dest, &header, sizeof(renderer::VertexStreamsHeader));

        Place<OffsetHandle, hlslpp::float3>(sub_mesh.positions, vertex_builder, header.Desc[renderer::VertexStream_Position]);
        Place<OffsetHandle, hlslpp::float3>(sub_mesh.normals, vertex_builder, header.Desc[renderer::VertexStream_Normal]);
        Place<OffsetHandle, hlslpp::float2>(sub_mesh.texCoords_0, vertex_builder, header.Desc[renderer::VertexStream_Texcoord0]);
        Place<OffsetHandle, hlslpp::float2>(sub_mesh.texCoords_1, vertex_builder, header.Desc[renderer::VertexStream_Texcoord1]);
        Place<OffsetHandle, hlslpp::float4>(sub_mesh.tangents, vertex_builder, header.Desc[renderer::VertexStream_Tangent]);
        Place<OffsetHandle, hlslpp::float3>(sub_mesh.colour, vertex_builder, header.Desc[renderer::VertexStream_Colour0]);
        Place<OffsetHandle, hlslpp::uint4>(sub_mesh.joints_0, vertex_builder, header.Desc[renderer::VertexStream_Joints0]);
        Place<OffsetHandle, hlslpp::float4>(sub_mesh.weights_0, vertex_builder, header.Desc[renderer::VertexStream_Weights0]);

        void* dest_index = index_builder.Place(sub_mesh_view.index_offset);
        std::memcpy(dest_index, sub_mesh.indices.data(), sizeof(uint32_t) * sub_mesh_view.index_count);
    }

    out_baked_mesh.vertex_buffer = vertex_builder.Finalize();
    out_baked_mesh.index_buffer = index_builder.Finalize();

    return true;
}