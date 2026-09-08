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
static bool ImportPrimitives(const cgltf_mesh& gltf_mesh, const cgltf_material* first_mtl, IntermediateMesh& mesh);
static void CopyIntegerAttributeToVector(std::vector<hlslpp::uint4>& out_vector, const cgltf_accessor* accessor);

template <typename VertexType>
static void CopyAttributeToVector(std::vector<VertexType>& out_vector, const cgltf_accessor* accessor);

Result<IntermediateModel> phx::AssetImporter::ImportGltfModel(const char* path)
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

    // Parsing materials will happen after simple cube test.
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
        IntermediateMesh& mesh = model.meshes.emplace_back();

        mesh.name = gltf_mesh.name
                        ? gltf_mesh.name
                        : "Mesh_" + std::to_string(name_mesh_count++);

        // Process Primitives
        ImportPrimitives(gltf_mesh, gltf_data->materials, mesh);

    }

    return true;
}

bool ImportPrimitives(const cgltf_mesh& gltf_mesh, const cgltf_material* first_mtl, IntermediateMesh& mesh)
{
    Span<cgltf_primitive> gltf_primitives(gltf_mesh.primitives, gltf_mesh.primitives_count);
    mesh.primitives.reserve(gltf_mesh.primitives_count);
    for (auto& gltf_prim : gltf_primitives)
    {
        IntermediateMesh::Primitive& prim = mesh.primitives.emplace_back();
        Span<cgltf_attribute> attributes(gltf_prim.attributes, gltf_prim.attributes_count);


        for (const auto& attribute : attributes)
        {
            switch (attribute.type)
            {
                case cgltf_attribute_type_position:
                    CopyAttributeToVector<hlslpp::float3>(
                        prim.positions,
                        attribute.data);
                    break;

                case cgltf_attribute_type_normal:
                    CopyAttributeToVector<hlslpp::float3>(
                        prim.normals,
                        attribute.data);
                    break;

                case cgltf_attribute_type_tangent:
                    CopyAttributeToVector<hlslpp::float4>(
                        prim.tangents,
                        attribute.data);
                    break;

                case cgltf_attribute_type_texcoord:
                    if (attribute.index == 0)
                    {
                        CopyAttributeToVector<hlslpp::float2>(
                            prim.texCoords_0,
                            attribute.data);
                    }
                    else if (attribute.index == 1)
                    {
                        CopyAttributeToVector<hlslpp::float2>(
                            prim.texCoords_1,
                            attribute.data);
                    }
                    else
                    {
                        PHX_LOG_WARN(
                            k_log,
                            "Unsupported texture coordinate set "
                            "TEXCOORD_{0} found.",
                            attribute.index);
                    }
                    break;

                case cgltf_attribute_type_color:
                    if (attribute.index == 0)
                    {
                        CopyAttributeToVector<hlslpp::float3>(
                            prim.colour,
                            attribute.data);
                    }
                    else
                    {
                        PHX_LOG_WARN(
                            k_log,
                            "Unsupported color set COLOR_{0} found.",
                            attribute.index);
                    }
                    break;

                case cgltf_attribute_type_joints:
                    if (attribute.index == 0)
                    {
                        CopyIntegerAttributeToVector(
                            prim.joints_0,
                            attribute.data);
                    }
                    else
                    {
                        PHX_LOG_WARN(
                            k_log,
                            "Unsupported joint set JOINTS_{0} found.",
                            attribute.index);
                    }
                    break;

                case cgltf_attribute_type_weights:
                    if (attribute.index == 0)
                    {
                        CopyAttributeToVector<hlslpp::float4>(
                            prim.weights_0,
                            attribute.data);
                    }
                    else
                    {
                        PHX_LOG_WARN(
                            k_log, 
                            "Unsupported weight set WEIGHTS_{0} found.",
                            attribute.index);
                    }
                    break;

                case cgltf_attribute_type_invalid:
                case cgltf_attribute_type_custom:
                default:
                    // TODO: Convert to a proper error message.
                    PHX_LOG_WARN(
                        k_log,
                        "Unhandled or invalid cgltf attribute type "
                        "encountered: {0}",
                        static_cast<uint32_t>(attribute.type));
                    break;
            }
        }

        // Handle indices separately
        if (gltf_prim.indices->count != 0)
        {
            cgltf_accessor_unpack_indices(
                gltf_prim.indices,
                prim.indices.data(),
                sizeof(uint32_t),
                gltf_prim.indices->count);
        }

        bool generated_normals = false;
        if (prim.normals.empty())
        {
            PHX_LOG_INFO(k_log, "Mesh doens't contain normal data. Generating normals.");
            PHX_ASSERT(false && "TODO: Generate normals");
            generated_normals = true;
        }

        const bool generate_tangents =
            gltf_prim.material && gltf_prim.material->normal_texture.texture && 
            (prim.tangents.empty() || generated_normals);

        if (generate_tangents || prim.tangents.empty())
        {
            PHX_LOG_INFO(k_log, "Generating tangent data.");
            PHX_LOG_WARN(k_log, "TODO: Generate tangents not implemented");
        }

        if (gltf_prim.material)
        {
            if (gltf_prim.material->alpha_mode == cgltf_alpha_mode_blend)
                prim.pso_flags |= PSOFlags::kAlphaBlend;

            if (gltf_prim.material->alpha_mode == cgltf_alpha_mode_mask)
                prim.pso_flags |= PSOFlags::kAlphaTest;

            if (gltf_prim.material->double_sided)
                prim.pso_flags |= PSOFlags::kTwoSided;

            prim.material_index =
                static_cast<uint32_t>(gltf_prim.material - first_mtl);
        }
    }
}


template <typename VertexType>
void CopyAttributeToVector(std::vector<VertexType>& out_vector, const cgltf_accessor *accessor)
{
    static_assert(sizeof(VertexType) == sizeof(float) * 4);
    const size_t num_components = cgltf_num_components(accessor->type);

    std::vector<float> temp_floats(accessor->count * num_components);
    cgltf_accessor_unpack_floats(accessor, temp_floats.data(), temp_floats.size());

    out_vector.resize(accessor->count);
    for (cgltf_size i = 0; i < accessor->count; ++i)
    {
        const float *source_floats = &temp_floats[i * num_components];
        void *dest_ptr = &out_vector[i];
        memcpy(dest_ptr, source_floats, num_components * sizeof(float));
    }
}

void CopyIntegerAttributeToVector(std::vector<hlslpp::uint4> &out_vector, const cgltf_accessor *accessor)
{
    out_vector.resize(accessor->count);

    // Determine how many components each vertex has (e.g., 4 for VEC4).
    size_t num_components = cgltf_num_components(accessor->type);

    for (cgltf_size i = 0; i < accessor->count; ++i)
    {
        // Create a temporary array to hold the integer components for one vertex.
        cgltf_uint components[4] = {0, 0, 0, 0};

        // cgltf_accessor_read_ui reads all integer components for the i-th
        // element and places them in our temporary array. It correctly
        // handles all source types like ubyte, ushort, etc.
        cgltf_accessor_read_uint(accessor, i, components, num_components);

        // Construct the final vector type from the integer components.
        // This assumes your hlslpp::uint4 (or similar) can be constructed this way.
        out_vector[i] = hlslpp::uint4(
            static_cast<uint32_t>(components[0]),
            static_cast<uint32_t>(components[1]),
            static_cast<uint32_t>(components[2]),
            static_cast<uint32_t>(components[3]));
    }
}