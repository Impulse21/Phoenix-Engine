#pragma once

#include <hlsl++.h>
#include <array>
#include <vector>

// TODO: Move this into it's own location
namespace phx::math
{
	// These structs are not packed
	struct BoundingSphere
	{
		hlslpp::float3 centre;
		hlslpp::float1 radius;
	};

	struct AxisAlignedBox
	{
		hlslpp::float3 min;
		hlslpp::float3 max;
	};
}

enum { kBaseColor, kMetallicRoughness, kOcclusion, kEmissive, kNormal, kNumTextures };

struct MaterialConstantData
{
	std::array<float, 4> base_colour_factor;
	std::array<float, 3> emissive_colour_factor; // default=[0,0,0]
	float normal_texture_scale; // default=1
	float metallic_factor; // default=1
	float roughness_factor; // default=1
	uint32_t flags;
};
struct MaterialTextureData
{
	uint16_t stringIdx[kNumTextures];
	uint32_t addressModes;
};

struct Mesh
{
	std::array<float, 4> bounds;           // A bounding sphere
	uint32_t             vb_offset;         // BufferLocation - Buffer.GpuVirtualAddress
	uint32_t             vb_size;           // SizeInBytes
	uint32_t             vb_depth_offset;   // BufferLocation - Buffer.GpuVirtualAddress
	uint32_t             vb_depth_size;     // SizeInBytes
	uint32_t             ib_offset;         // BufferLocation - Buffer.GpuVirtualAddress
	uint32_t             ib_size;           // SizeInBytes
	uint8_t              vb_stride;         // StrideInBytes
	uint8_t              ib_format;         // DXGI_FORMAT
	uint16_t             mesh_cbv;          // Index of mesh constant buffer
	uint16_t             material_cbv;      // Index of material constant buffer
	uint16_t             srv_table;         // Offset into SRV descriptor heap for textures
	uint16_t             sampler_table;     // Offset into sampler descriptor heap for samplers
	uint16_t             pso_flags;         // Flags needed to request a PSO
	uint16_t             pso;               // Index of pipeline state object
	uint16_t             num_joints;        // Number of skeleton joints when skinning
	uint16_t             start_joint;       // Flat offset to first joint index
	uint16_t             num_draws;         // Number of draw groups

	struct Draw
	{
		uint32_t prim_count;    // Number of indices = 3 * number of triangles
		uint32_t start_index;   // Offset to first index in index buffer
		uint32_t base_vertex;   // Offset to first vertex in vertex buffer
	};
	Draw draw[1];               // Actually 1 or more draws
};

struct ModelData
{
	std::vector<std::byte> geometry_data;
	std::vector<MaterialTextureData> material_textures;
	std::vector<MaterialConstantData> material_constants;
	std::vector<Mesh*> meshes;

	// TODO: Animation and skinning data
	std::vector<hlslpp::float4x4> transforms;
	std::vector<std::string> texture_names;
	std::vector<uint8_t> texture_options;

};