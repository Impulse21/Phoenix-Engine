#include "PhxResourceCompiler_pch.h"

#include <PhxResourceCompiler/Mesh/MeshImporter_Gltf.h>

#include <PhxCore/Math.h>

#include <cgltf.h>

using namespace phx;
using namespace phx::resource;
using namespace phx::resource::importer;

using namespace hlslpp;

static void InitializeSubMesh(compiler::RawSubMesh& sub_mesh, cgltf_primitive const& src_prim, Span<cgltf_material> materials);
static void CalculateBounds(compiler::RawSubMesh& sub_mesh);

static void CopyIntegerAttributeToVector(std::vector<hlslpp::uint4>& out_vector, const cgltf_accessor* accessor);

template <typename VertexType>
static void CopyAttributeToVector(std::vector<VertexType>& out_vector, const cgltf_accessor* accessor);

Result<compiler::RawMesh> importer::ImportMesh(const cgltf_mesh& gltf_mesh, Span<cgltf_material> materials)
{   
	compiler::RawMesh raw_mesh = {};
	
	Span<cgltf_primitive> primitives(
        &gltf_mesh.primitives[0],
        gltf_mesh.primitives_count);

        
	raw_mesh.sub_meshes.reserve(primitives.Size());
	
    for (auto& primitive : primitives)
	{
		compiler::RawSubMesh& sub_mesh = raw_mesh.sub_meshes.emplace_back();

		InitializeSubMesh(sub_mesh, primitive, materials);
		CalculateBounds(sub_mesh);
	}
	
	return raw_mesh;
}


static void InitializeSubMesh(compiler::RawSubMesh& sub_mesh, cgltf_primitive const& src_prim, Span<cgltf_material> materials)
{
	Span<cgltf_attribute> attributes(src_prim.attributes, src_prim.attributes_count);
	compiler::VertexBufferStreams& vertex_streams = sub_mesh.vertex_streams;
	for (const auto& attribute : attributes)
	{
		switch (attribute.type)
		{
		case cgltf_attribute_type_position:
			vertex_streams.positions = std::make_unique<std::vector<hlslpp::float3>>();
			CopyAttributeToVector<hlslpp::float3>(*vertex_streams.positions, attribute.data);
			break;

		case cgltf_attribute_type_normal:
			vertex_streams.normals = std::make_unique<std::vector<hlslpp::float3>>();
			CopyAttributeToVector<hlslpp::float3>(*vertex_streams.normals, attribute.data);
			break;

		case cgltf_attribute_type_tangent:
			vertex_streams.tangents = std::make_unique<std::vector<hlslpp::float4>>();
			CopyAttributeToVector<hlslpp::float4>(*vertex_streams.tangents, attribute.data);
			break;

		case cgltf_attribute_type_texcoord:
			if (attribute.index == 0)
			{
				sub_mesh.vertex_streams.texCoords_0 = std::make_unique<std::vector<hlslpp::float2>>();
				CopyAttributeToVector<hlslpp::float2>(*vertex_streams.texCoords_0, attribute.data);
			}
			else if (attribute.index == 1)
			{
				sub_mesh.vertex_streams.texCoords_1 = std::make_unique<std::vector<hlslpp::float2>>();
				CopyAttributeToVector<hlslpp::float2>(*vertex_streams.texCoords_1, attribute.data);
			}
			else
			{
				PHX_CORE_WARN("Unsupported texture coordinate set TEXCOORD_{0} found.", attribute.index);
			}
			break;

		case cgltf_attribute_type_color:
			if (attribute.index == 0)
			{
				sub_mesh.vertex_streams.colour = std::make_unique<std::vector<hlslpp::float3>>();
				CopyAttributeToVector<hlslpp::float3>(*vertex_streams.colour, attribute.data);
			}
			else
			{
				PHX_CORE_WARN("Unsupported color set COLOR_{0} found.", attribute.index);
			}
			break;

		case cgltf_attribute_type_joints:
			if (attribute.index == 0)
			{
				sub_mesh.vertex_streams.joints_0 = std::make_unique<std::vector<hlslpp::uint4>>();
				CopyIntegerAttributeToVector(*vertex_streams.joints_0, attribute.data);
			}
			else
			{
				PHX_CORE_WARN("Unsupported joint set JOINTS_{0} found.", attribute.index);
			}
			break;

		case cgltf_attribute_type_weights:
			if (attribute.index == 0)
			{
				sub_mesh.vertex_streams.weights_0 = std::make_unique<std::vector<hlslpp::float4>>();
				CopyAttributeToVector<hlslpp::float4>(*vertex_streams.weights_0, attribute.data);
			}
			else
			{
				PHX_CORE_WARN("Unsupported weight set WEIGHTS_{0} found.", attribute.index);
			}
			break;

		case cgltf_attribute_type_invalid:
		case cgltf_attribute_type_custom:
		default:
			// TODO: Convert to a proper error message.
			PHX_CORE_WARN("Unhandled or invalid cgltf attribute type encountered: {0}", static_cast<uint32_t>(attribute.type));
			break;
		}
	}

	// Handle indices separately
	if (src_prim.indices->count != 0) 
	{
		sub_mesh.indices = std::make_unique<std::vector<uint32_t>>(src_prim.indices->count);
		cgltf_accessor_unpack_indices(src_prim.indices, sub_mesh.indices->data(), sizeof(uint32_t), src_prim.indices->count);
	}

	bool generated_normals = false;
	if (!vertex_streams.normals || vertex_streams.normals->empty())
	{
		PHX_CORE_INFO("Mesh doens't contain normal data. Generating normals.");
		PHX_CORE_ASSERT(false, "TODO: Generate normals");
		generated_normals = true;
	}

	const bool generate_tangents =
		src_prim.material &&
		src_prim.material->normal_texture.texture &&
		(!vertex_streams.tangents || vertex_streams.tangents->empty() || generated_normals);

	if (generate_tangents || !vertex_streams.tangents || vertex_streams.tangents->empty())
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

		sub_mesh.material_index = static_cast<uint32_t>(src_prim.material - materials.begin());
	}
}

static void CalculateBounds(compiler::RawSubMesh& sub_mesh)
{
	PHX_ASSERT(sub_mesh.vertex_streams.positions && !sub_mesh.vertex_streams.positions->empty());
	const std::vector<hlslpp::float3>& position_stream = *sub_mesh.vertex_streams.positions;

	float3 min_position(std::numeric_limits<float>::max());
	float3 max_position(std::numeric_limits<float>::min());
	for (auto& position : position_stream)
	{
		min_position = hlslpp::min(min_position, position);
		max_position = hlslpp::max(max_position, position);
	}

	float3 sphere_centre_ls = min_position + max_position * 0.5f;
	float1 max_radius_ls_sq;

	for (auto& position : position_stream)
	{
		float1 length_sqrt_ls = hlslpp::length(sphere_centre_ls - position);
		max_radius_ls_sq = hlslpp::max(max_radius_ls_sq, length_sqrt_ls);

		sub_mesh.bbox_ls.AddPoint(position);
	}

	sub_mesh.bounds_ls = math::BoundingSphere(sphere_centre_ls, hlslpp::sqrt(max_radius_ls_sq));
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
