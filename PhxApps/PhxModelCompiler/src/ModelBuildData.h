#pragma once

#include <PhxCore/Base.h>
#include <hlsl++.h>
#include <array>
#include <vector>

// TODO: Move this into it's own location
namespace phx::math
{
	// These structs are not packed
	struct BoundingSphere
	{
		hlslpp::float3 centre = hlslpp::float3(0.0f);
		hlslpp::float1 radius = hlslpp::float1(0.0f);

		BoundingSphere Union(const BoundingSphere& rhs) const
		{
			using namespace hlslpp;

			float1 rad_a = radius;
			if (rad_a == 0.0f)
				return rhs;

			float1 rad_b = rhs.radius;
			if (rad_b == 0.0f)
				return *this;

			float3 diff = centre - rhs.centre;
			float1 dist = length(diff);

			// Safe normalize vector between sphere centers
			diff = (float)dist < 1e-6f ? float3(1.0, 0.0, 0.0) : diff * rcp(dist);

			float3 extreme_a = centre + diff * hlslpp::max(rad_a, rad_b - dist);
			float3 extreme_b = rhs.centre - diff * hlslpp::max(rad_b, rad_a - dist);

			return {
				.centre = (extreme_a + extreme_b) * 0.5f,
				.radius = length(extreme_a - extreme_b) * 0.5f
			};
		}
	};

	struct AxisAlignedBox
	{
		hlslpp::float3 min = hlslpp::float3(0.0f);
		hlslpp::float3 max = hlslpp::float3(0.0f);

		void AddPoint(hlslpp::float3 point)
		{
			min = hlslpp::min(point, min);
			max = hlslpp::max(point, max);
		}

		void AddBoundingBox(AxisAlignedBox const& box)
		{
			AddPoint(box.min);
			AddPoint(box.max);
		}
	};
}

enum { kBaseColor, kMetallicRoughness, kOcclusion, kEmissive, kNormalMap, kNumTextures };

// -- TODO: Move to texture compiler
namespace TexConversionFlags
{
	enum
	{
		kSRGB = 1,          // Texture contains sRGB colors
		kPreserveAlpha = 2, // Keep four channels
		kNormalMap = 4,     // Texture contains normals
		kBumpToNormal = 8,  // Generate a normal map from a bump map
		kDefaultBC = 16,    // Apply standard block compression (BC1-5)
		kQualityBC = 32,    // Apply quality block compression (BC6H/7)
		kFlipVertical = 64,
	};
}

inline uint8_t TextureOptions(bool sRGB, bool hasAlpha = false, bool invertY = false)
{
	return (sRGB ? TexConversionFlags::kSRGB : 0) | (hasAlpha ? TexConversionFlags::kPreserveAlpha : 0) | (invertY ? TexConversionFlags::kFlipVertical : 0);
}
// -- end TODO

namespace PSOFlags
{
	enum : uint16_t
	{
		kAlphaBlend = BIT(1),
		kAlphaTest = BIT(2),
		kTwoSided = BIT(3),
	};
}
struct Mesh
{
	std::array<float, 4> bounds;           // A bounding sphere
	uint32_t             vb_offset;         // BufferLocation - Buffer.GpuVirtualAddress
	uint32_t             vb_size;           // SizeInBytes
	uint32_t             ib_offset;         // BufferLocation - Buffer.GpuVirtualAddress
	uint32_t             ib_size;           // SizeInBytes
	uint8_t              ib_format;         // DXGI_FORMAT
	uint16_t             mesh_cbv;          // Index of mesh constant buffer
	uint16_t             material_cbv;      // Index of material constant buffer
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


struct MaterialTextureData
{
	std::string name = "";

	enum class WrapMode
	{
		ClampToEdge = 0,
		MirroredRepeat,
		Repeat
	};
	WrapMode wrap_s;
	WrapMode wrap_t;
};

struct MaterialData
{
	std::string id;
	std::array<float, 4> base_colour_factor;
	std::array<float, 3> emissive_factor; // default=[0,0,0]
	float normal_texture_scale; // default=1
	float metallic_factor; // default=1
	float roughness_factor; // default=1
	union
	{
		uint32_t flags;
		struct
		{
			uint32_t baseColorUV : 1;
			uint32_t metallic_roughness_uv : 1;
			uint32_t occlusion_uv : 1;
			uint32_t emissive_uv : 1;
			uint32_t normal_uv : 1;
			uint32_t two_sided : 1;
			uint32_t alpha_test : 1;
			uint32_t alpha_blend : 1;
			uint32_t _pad : 8;
			uint32_t alpha_cutoff : 16; // FP16
		};
	};;

	std::array<MaterialTextureData, kNumTextures> texture_data;
};

struct ModelData
{
	std::string name;
	phx::math::BoundingSphere bounding_sphere;
	phx::math::AxisAlignedBox bounding_box;

	std::vector<std::byte> geometry_data;
	std::vector<std::unique_ptr<::Mesh>> meshes;

	std::vector<MaterialData> material_dependencies;
};