#pragma once

#include <PhxCore/Base.h>

namespace phx::rhi
{

	enum class ReferenceType : unsigned int
	{
		Invalid,
		RenderTarget,
		PassResult,
		ExternalSRV

	};

	struct Reference
	{
		enum : uint8_t
		{
			AllSubresources = 0x7f
		};

		union
		{
			struct
			{
				ReferenceType Type : 2;
				unsigned int Depth : 1;
				unsigned int EntireTexture : 1;		 // Used for PassResult types to specify AllSubresources (since SubResource is used for the output index)
				unsigned int MipSlice : 1;
				unsigned int SubResource : 7;
				unsigned int MipLevel : 4;
				unsigned int Index : 16;
			};
			unsigned int Data;
		};

		Reference() {}

		constexpr Reference(const ReferenceType type, const unsigned int index, const unsigned int sub_resource, const unsigned int depth = 0, const unsigned int entire_texture = 0, const unsigned int mip_slice = 0, const unsigned int mip_level = 0) :
			Type(type),
			Depth(depth),
			EntireTexture(entire_texture),
			MipSlice(mip_slice),
			SubResource(sub_resource),
			MipLevel(mip_level),
			Index(index)
		{
			PHX_ASSERT(sub_resource < 128);
			PHX_ASSERT(index < 65536);
			PHX_ASSERT(mip_slice == 1 || mip_level == 0);
			PHX_ASSERT(mip_level < 16);
		}

		constexpr Reference SubResourceRef(const unsigned int sub_resource) const { return Reference(Type, Index, sub_resource, Depth); }
		constexpr Reference MipSliceRef(const unsigned int mip_level) const { return Reference(Type, Index, 0, Depth, 0, 1, mip_level); }

		static constexpr Reference Null()
		{
			return Reference(ReferenceType::Invalid, 0, 0);
		}

		explicit operator bool() const { return Data != 0; }
	};

	struct RenderTarget
	{
		int Index;

		RenderTarget() {}
		explicit RenderTarget(const int index) : Index(index) {}

		constexpr operator Reference() const { return Reference(ReferenceType::RenderTarget, Index, 0); }
		constexpr Reference SubResource(const unsigned int sub_resource) const { return Reference(ReferenceType::RenderTarget, Index, sub_resource); }
		constexpr Reference MipSlice(const unsigned int mip_level) const { return Reference(ReferenceType::RenderTarget, Index, 0, 0, 0, 1, mip_level); }
		constexpr Reference AllSubResources() const { return SubResource(Reference::AllSubresources); }
	};

	struct PassResult
	{
		int Index;

		PassResult() {}
		explicit PassResult(const int index) : Index(index) {}

		constexpr Reference Colour(const int index, const bool entire_texture = false) const { return Reference(ReferenceType::PassResult, Index, index, 0, entire_texture); }
		constexpr Reference Depth(const bool entire_texture = false) const { return Reference(ReferenceType::PassResult, Index, 0, 1, entire_texture); }
	};

#if false // Some Render Graph code example
	const Graphics::ResourceDesc& source_desc = render_graph->Desc(source_ref, &source_dim);

	Graphics::cRenderGraph::RenderTarget temp_target = render_graph->CreateRenderTarget(
		"GaussianBlurTemp",
		Graphics::ResourceDesc(UInt2(source_dim.x, source_dim.y), source_desc.Format),
		Graphics::cRenderGraph::ResourceContent::Undefined);

	render_graph->BeginPass("GaussianBlurHorizontal");
	render_graph->TrivialPass();

	render_graph->WriteColour(temp_target);
	render_graph->Read(source_ref);
	if (depth_ref)
	{
		render_graph->Read(depth_ref);
	}
	if (gbuffer0_ref)
	{
		render_graph->Read(gbuffer0_ref);
	}

	Graphics::cRenderGraph::PassResult horizontal_result = render_graph->EndPass([this, is_bilateral = depth_ref || gbuffer0_ref, camera, depth_threshold, normal_threshold](Graphics::cRenderContext* render_context, const Graphics::cPassResources& pass_resources)
		{
			render_context->SetBindingSchema(GaussianBlurBinding::Schema);
			render_context->SetRenderState(m_HorizontalRenderState[(camera && camera->IsOrthographic()) ? 1 : 0]);

			Graphics::ScopedShaderResourceTable table(render_context, GaussianBlurBinding::Schema, GaussianBlurBinding::SRVs(), pass_resources.SRVRanges(), pass_resources.NumSRVRanges());
			Graphics::ScopedDynamicConstantBuffer<GaussianBlurParams> params;

			if (table && (!is_bilateral || params.Init(render_context, GaussianBlurBinding::Params(), m_ConstantBufferView, [&](GaussianBlurParams* params)
				{
					const Matrix& projection = camera->Projection();
					params->ProjectionParams = Float2(projection.Element(3, 2), projection.Element(2, 2));
					params->DepthThreshold = depth_threshold;
					params->NormalThreshold = normal_threshold;
				})))
			{
				PostProcessUtil::DrawFullScreenQuad(render_context);
			}
		});

	render_graph->BeginPass("GaussianBlurVertical");
	render_graph->TrivialPass();

	render_graph->WriteColour(source_ref);
	render_graph->Read(horizontal_result.Colour(0));

	return render_graph->EndPass([this, is_bilateral = depth_ref || gbuffer0_ref, camera, depth_threshold, normal_threshold](Graphics::cRenderContext* render_context, const Graphics::cPassResources& pass_resources)
		{
			render_context->SetBindingSchema(GaussianBlurBinding::Schema);
			render_context->SetRenderState(m_VerticalRenderState[(camera && camera->IsOrthographic()) ? 1 : 0]);

			Graphics::ScopedShaderResourceTable table(render_context, GaussianBlurBinding::Schema, GaussianBlurBinding::SRVs(), pass_resources.SRVRanges(), pass_resources.NumSRVRanges());
			Graphics::ScopedDynamicConstantBuffer<GaussianBlurParams> params;

			if (table && (!is_bilateral || params.Init(render_context, GaussianBlurBinding::Params(), m_ConstantBufferView, [&](GaussianBlurParams* params)
				{
					const Matrix& projection = camera->Projection();
					params->ProjectionParams = Float2(projection.Element(3, 2), projection.Element(2, 2));
					params->DepthThreshold = depth_threshold;
					params->NormalThreshold = normal_threshold;
				})))
			{
				PostProcessUtil::DrawFullScreenQuad(render_context);
			}
		});
#endif 
}