#include "GltfImporter.h"

#include <PhxEngine/Core/Span.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/VFS/VFS.h>
#include <cgltf.h>

using namespace phx;

namespace
{
constexpr Log::Channel k_log = {"GltfImporter"};
}

static bool ImportMeshes(const cgltf_data* gltf_data, IntermediateModel& model);
static bool ImportPrimitives(Span<cgl gltf_mesh, IntermediateMesh& mesh)

Result<IntermediateModel> phx::ImportGltfMesh(const char* path)
{
    IntermediateModel model;

    MemoryBuffer source = VFS::ReadFile(path);
    if (source.IsEmpty())
    {
        PHX_LOG_ERROR(k_log, "Could not read glTF file '{0}'", path);
        return phx::Unexpected(phx::ResultError::Failure);
    }

    cgltf_options options = {
    // TODO: Implement these for external file loading.
#if false
        .file = {
            .read = &CgltfReadFile,
            .release = &CgltfReleaseFile,
        }
#endif
    };

    cgltf_data* gltf_data = nullptr;
    cgltf_result result =
        cgltf_parse(&options, source.Data(), source.Size(), &gltf_data);
    if (result != cgltf_result_success)
    {
        PHX_LOG_ERROR(k_log, "Couldn't parse glTF file '{0}'", path);
        return phx::Unexpected(phx::ResultError::Failure);
    }

    result = cgltf_load_buffers(&options, gltf_data, path);
    if (result != cgltf_result_success)
    {
        PHX_LOG_ERROR(k_log, "Couldn't load glTF file '{0}'", path);
        return phx::Unexpected(phx::ResultError::Failure);
    }

    // Parse meshes
    // Could this be multi threaded?
    ImportMeshes(gltf_data, model);
    // Parse Textures
    // Parse Materials

    return model;
}

bool ImportMeshes(const cgltf_data* gltf_data, IntermediateModel& model)
{
    uint32_t name_mesh_count = 0;

    for (size_t i = 0; i < gltf_data->meshes_count; ++i)
    {
        const cgltf_mesh& gltf_mesh = gltf_data->meshes[i];

        // build mesh name
        IntermediateMesh mesh;

        mesh.name = gltf_mesh.name
                        ? gltf_mesh.name
                        : "Mesh_" + std::to_string(name_mesh_count++);

        // Process Primitives
        mesh.primitives.reserve(gltf_mesh.primitives_count);
        ImportPrimitives(gltf_mesh, mesh);

    }

    return true;
}

bool ImportPrimitives(const cgltf_mesh& gltf_mesh, IntermediateMesh& mesh)
{

        for (size_t i = 0; i < gltf_mesh.primitives_count; ++i)
        {
    IntermediateMesh::Primitive& prim = mesh.primitives.emplace_back();
    const cgltf_primitive& gltf_prim =
        gltf_mesh.primitives[iPrim]

        Span<cgltf_attribute>
            attributes(src_prim.attributes, src_prim.attributes_count);
    compiler::VertexBufferStreams& vertex_streams = sub_mesh.vertex_streams;
    for (const auto& attribute : attributes)
    {
        switch (attribute.type)
        {
            case cgltf_attribute_type_position:
                vertex_streams.positions =
                    std::make_unique<std::vector<hlslpp::float3>>();
                CopyAttributeToVector<hlslpp::float3>(*vertex_streams.positions,
                                                      attribute.data);
                break;

            case cgltf_attribute_type_normal:
                vertex_streams.normals =
                    std::make_unique<std::vector<hlslpp::float3>>();
                CopyAttributeToVector<hlslpp::float3>(*vertex_streams.normals,
                                                      attribute.data);
                break;

            case cgltf_attribute_type_tangent:
                vertex_streams.tangents =
                    std::make_unique<std::vector<hlslpp::float4>>();
                CopyAttributeToVector<hlslpp::float4>(*vertex_streams.tangents,
                                                      attribute.data);
                break;

            case cgltf_attribute_type_texcoord:
                if (attribute.index == 0)
                {
                    sub_mesh.vertex_streams.texCoords_0 =
                        std::make_unique<std::vector<hlslpp::float2>>();
                    CopyAttributeToVector<hlslpp::float2>(
                        *vertex_streams.texCoords_0, attribute.data);
                }
                else if (attribute.index == 1)
                {
                    sub_mesh.vertex_streams.texCoords_1 =
                        std::make_unique<std::vector<hlslpp::float2>>();
                    CopyAttributeToVector<hlslpp::float2>(
                        *vertex_streams.texCoords_1, attribute.data);
                }
                else
                {
                    PHX_CORE_WARN(
                        "Unsupported texture coordinate set "
                        "TEXCOORD_{0} found.",
                        attribute.index);
                }
                break;

            case cgltf_attribute_type_color:
                if (attribute.index == 0)
                {
                    sub_mesh.vertex_streams.colour =
                        std::make_unique<std::vector<hlslpp::float3>>();
                    CopyAttributeToVector<hlslpp::float3>(
                        *vertex_streams.colour, attribute.data);
                }
                else
                {
                    PHX_CORE_WARN("Unsupported color set COLOR_{0} found.",
                                  attribute.index);
                }
                break;

            case cgltf_attribute_type_joints:
                if (attribute.index == 0)
                {
                    sub_mesh.vertex_streams.joints_0 =
                        std::make_unique<std::vector<hlslpp::uint4>>();
                    CopyIntegerAttributeToVector(*vertex_streams.joints_0,
                                                 attribute.data);
                }
                else
                {
                    PHX_CORE_WARN("Unsupported joint set JOINTS_{0} found.",
                                  attribute.index);
                }
                break;

            case cgltf_attribute_type_weights:
                if (attribute.index == 0)
                {
                    sub_mesh.vertex_streams.weights_0 =
                        std::make_unique<std::vector<hlslpp::float4>>();
                    CopyAttributeToVector<hlslpp::float4>(
                        *vertex_streams.weights_0, attribute.data);
                }
                else
                {
                    PHX_CORE_WARN("Unsupported weight set WEIGHTS_{0} found.",
                                  attribute.index);
                }
                break;

            case cgltf_attribute_type_invalid:
            case cgltf_attribute_type_custom:
            default:
                // TODO: Convert to a proper error message.
                PHX_CORE_WARN(
                    "Unhandled or invalid cgltf attribute type "
                    "encountered: {0}",
                    static_cast<uint32_t>(attribute.type));
                break;
        }
    }

    // Handle indices separately
    if (src_prim.indices->count != 0)
    {
        sub_mesh.indices =
            std::make_unique<std::vector<uint32_t>>(src_prim.indices->count);
        cgltf_accessor_unpack_indices(
            src_prim.indices, sub_mesh.indices->data(), sizeof(uint32_t),
            src_prim.indices->count);
    }

    bool generated_normals = false;
    if (!vertex_streams.normals || vertex_streams.normals->empty())
    {
        PHX_CORE_INFO("Mesh doens't contain normal data. Generating normals.");
        PHX_CORE_ASSERT(false, "TODO: Generate normals");
        generated_normals = true;
    }

    const bool generate_tangents =
        src_prim.material && src_prim.material->normal_texture.texture &&
        (!vertex_streams.tangents || vertex_streams.tangents->empty() ||
         generated_normals);

    if (generate_tangents || !vertex_streams.tangents ||
        vertex_streams.tangents->empty())
    {
        PHX_CORE_INFO("Generating tangent data.");
        PHX_CORE_WARN("TODO: Generate tangents not implemented");
    }

    if (src_prim.material)
    {
        if (src_prim.material->alpha_mode == cgltf_alpha_mode_blend)
            sub_mesh.pso_flags |= compiler::PSOFlags::kAlphaBlend;

        if (src_prim.material->alpha_mode == cgltf_alpha_mode_mask)
            sub_mesh.pso_flags |= compiler::PSOFlags::kAlphaTest;

        if (src_prim.material->double_sided)
            sub_mesh.pso_flags |= compiler::PSOFlags::kTwoSided;

        sub_mesh.material_index =
            static_cast<uint32_t>(src_prim.material - materials.begin());
    }
}
}