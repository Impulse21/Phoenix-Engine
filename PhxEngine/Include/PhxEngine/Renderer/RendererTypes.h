#pragma once

#include <PhxEngine/Core/Handle.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include <array>
#include <DirectXMath.h>

namespace PhxEngine::Renderer
{
	struct MaterialDesc
	{

	};

	struct RenderMaterial;
	using RenderMaterialHandle = Core::Handle<RenderMaterial>;

	struct MeshDesc
	{
		// Material
		RenderMaterialHandle Material;

		// Buffer Data
		std::vector<uint32_t> Indices;
		std::vector<float> VertexBufferData;

		// TODO: Handle
		
		// Meshlet Data
		enum class VertexAttribute : uint8_t
		{
			Position = 0,
			TexCoord,
			Normal,
			Tangent,
			Colour,
			Count,
		};

		std::array<RHI::BufferRange, (int)VertexAttribute::Count> BufferRanges;

		// -- Helper Functions ---
		[[nodiscard]] bool HasVertexAttribuite(VertexAttribute attr) const { return this->BufferRanges[(int)attr].SizeInBytes != 0; }
		RHI::BufferRange& GetVertexAttribute(VertexAttribute attr) { return this->BufferRanges[(int)attr]; }
		[[nodiscard]] const RHI::BufferRange& GetVertexAttribute(VertexAttribute attr) const { return this->BufferRanges[(int)attr]; }
	};

	struct Drawable;
	using DrawableHandle = Core::Handle<Drawable>;

	struct DrawableInstanceDesc
	{
		enum RenderBucket
		{
			RenderType_Void = 0,
			RenderType_Opaque = 1 << 0,
			RenderType_Transparent = 1 << 1,
			RenderType_Shadow = 1 << 2,
			RenderType_OpaqueAll = RenderType_Opaque | RenderType_Shadow,
			RenderType_TransparentAll = RenderType_Opaque | RenderType_Shadow
		};

		DrawableHandle Drawable;
		DirectX::XMFLOAT4 Color = DirectX::XMFLOAT4(1, 1, 1, 1);
		DirectX::XMFLOAT4 EmissiveColor = DirectX::XMFLOAT4(1, 1, 1, 1);
	};
	struct DrawableInstace;
	using DrawableInstaceHandle = Core::Handle<DrawableInstace>;
}