#include <PhxEngine/Renderer/FontRenderer.h>

#include <PhxEngine/Core/Asserts.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/RHI/Dx12/RootSignatureBuilder.h>
#include <PhxEngine/Renderer/Renderer.h>
#include <Shaders/FontVS_compiled.h>
#include <Shaders/FontPS_compiled.h>

#include "../Utility/ArialTTData.h"

// DX12 Specific
#include <dxgi.h>

using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Renderer;

FontRenderer::FontRenderer(
	RHI::IGraphicsDevice* graphicsDevice)
	: m_graphicsDevice(graphicsDevice)
{
	this->LoadShaderData();
	
	// Add Default Font
	this->AddFontStyle("arial", arial, sizeof(arial));
}

void PhxEngine::Renderer::FontRenderer::DrawString(
	std::string const& text,
	DrawParams const& params,
	RHI::CommandListHandle cmdList)
{
	this->DrawStringInternal(text.c_str(), text.size(), params, cmdList);
}

void PhxEngine::Renderer::FontRenderer::DrawString(std::wstring const& text, DrawParams const& params, RHI::CommandListHandle cmdList)
{
	this->DrawStringInternal(text.c_str(), text.size(), params, cmdList);
}

void PhxEngine::Renderer::FontRenderer::DrawString(const char* text, DrawParams const& params, RHI::CommandListHandle cmdList)
{
	size_t textLength = strlen(text);
	if (textLength == 0)
	{
		return;
	}

	this->DrawStringInternal(text, textLength, params, cmdList);
}

void PhxEngine::Renderer::FontRenderer::DrawString(const wchar_t* text, DrawParams const& params, RHI::CommandListHandle cmdList)
{
	size_t textLength = wcslen(text);
	if (textLength == 0)
	{
		return;
	}

	this->DrawStringInternal(text, textLength, params, cmdList);
}

size_t PhxEngine::Renderer::FontRenderer::AddFontStyle(std::string const& name, const uint8_t* data, size_t size)
{
	for (int i = 0; i < this->m_fontStyles.size(); i++)
	{
		auto& fontStyle = this->m_fontStyles[i];
		if (fontStyle.Name == name)
		{
			return i;
		}
	}

	this->m_fontStyles.emplace_back();
	this->m_fontStyles.back().Create(name, data, size);
	return this->m_fontStyles.size() - 1;
}

void PhxEngine::Renderer::FontRenderer::UpdatePendingGlyphs()
{
	if (this->m_pendingGlyphs.empty())
	{
		return;
	}

	// Pad the glyph rects in the atlas to avoid bleeding from nearby texels:
	const int borderPadding = 1;

	// Font resolution is upscaled to make it sharper:
	const float upscaling = 2.0f;

	for (int32_t hash : this->m_pendingGlyphs)
	{
		const int code = this->GetCodeFromHash(hash);
		const int style = this->GetStyleFromHash(hash);
		const float height = (float)this->GetHeightFromHash(hash) * upscaling;
		FontStyle& fontStyle = this->m_fontStyles[style];

		float fontScaling = stbtt_ScaleForPixelHeight(&fontStyle.FontInfo, height);
		// get bounding box for character (may be offset to account for chars that dip above or below the line
		int left;
		int top;
		int right;
		int bottom;
		stbtt_GetCodepointBitmapBox(&fontStyle.FontInfo, code, fontScaling, fontScaling, &left, &top, &right, &bottom);

		// Glyph dimensions are calculated without padding:
		Glyph& glyph = this->m_glyphLookup[hash];
		glyph.X = float(left);
		glyph.Y = float(top) + float(fontStyle.Ascent) * fontScaling;
		glyph.Width = float(right - left);
		glyph.Height = float(bottom - top);

		// Remove dpi upscaling:
		glyph.X = glyph.X / upscaling;
		glyph.Y = glyph.Y / upscaling;
		glyph.Width = glyph.Width / upscaling;
		glyph.Height = glyph.Height / upscaling;

		// Add padding to the rectangle that will be packed in the atlas:
		right += borderPadding * 2;
		bottom += borderPadding * 2;
		this->m_rectLookup[hash] = RHI::Rect(left, right, bottom, top);
	}
	this->m_pendingGlyphs.clear();


}

void PhxEngine::Renderer::FontRenderer::LoadShaderData()
{
	// Load Shaders
	ShaderDesc shaderDesc = {};
	shaderDesc.DebugName = "Font Vertex Shader";
	shaderDesc.ShaderType = EShaderType::Vertex;

	ShaderHandle vs = this->m_graphicsDevice->CreateShader(shaderDesc, gFontVS, _countof(gFontVS));

	shaderDesc.DebugName = "Font Pixel Shader";
	shaderDesc.ShaderType = EShaderType::Pixel;

	ShaderHandle ps = this->m_graphicsDevice->CreateShader(shaderDesc, gFontPS, _countof(gFontPS));

	GraphicsPSODesc psoDesc = {};
	psoDesc.VertexShader = vs;
	psoDesc.PixelShader = ps;

	std::vector<VertexAttributeDesc> attributeDesc =
	{
		{ "POSITION", 0, EFormat::RG32_FLOAT, 0, VertexAttributeDesc::SAppendAlignedElement, false},
		{ "TEXCOORD", 0, EFormat::RG32_FLOAT, 0, VertexAttributeDesc::SAppendAlignedElement, false},
	};

	InputLayoutHandle inputLayout = this->m_graphicsDevice->CreateInputLayout(attributeDesc.data(), attributeDesc.size());

	psoDesc.InputLayout = inputLayout;

	psoDesc.RasterRenderState = {};
	psoDesc.RasterRenderState.FillMode;
	psoDesc.RasterRenderState.CullMode = RasterCullMode::Front;
	psoDesc.RasterRenderState.FrontCounterClockwise = true;

	psoDesc.DepthStencilRenderState = {};
	psoDesc.DepthStencilRenderState.DepthTestEnable = false;
	psoDesc.DepthStencilRenderState.StencilEnable = false;

	psoDesc.PrimType = PrimitiveType::TriangleStrip;

	psoDesc.BlendRenderState = {};
	auto& renderTarget = psoDesc.BlendRenderState.Targets[0];
	renderTarget.BlendEnable = true;
	renderTarget.DestBlend = BlendFactor::InvSrcAlpha;
	renderTarget.DestBlendAlpha = BlendFactor::InvSrcAlpha;

	// TODO: Work around to get things working. I really shouldn't even care making my stuff abstract. But here we are.
	Dx12::RootSignatureBuilder builder = {};
	builder.Add32BitConstants<999, 0>(sizeof(Shader::FontDrawInfo) / 4);

	// Default Sampler
	builder.AddStaticSampler<0, 0>(
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0u,
		D3D12_COMPARISON_FUNC_ALWAYS,
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK);

	Dx12::DescriptorTable bindlessDescriptorTable = {};
	Renderer::FillBindlessDescriptorTable(this->m_graphicsDevice, bindlessDescriptorTable);
	builder.AddDescriptorTable(bindlessDescriptorTable);

	psoDesc.RootSignatureBuilder = &builder;

	this->m_graphicsDevice->CreateGraphicsPSOHandle(psoDesc);
}

void PhxEngine::Renderer::FontRenderer::FontStyle::Create(std::string const& name, const uint8_t* data, size_t size)
{
	this->Name = name;
	int offset = stbtt_GetFontOffsetForIndex(data, 0);

	if (!stbtt_InitFont(&this->FontInfo, data, size))
	{
		LOG_CORE_ERROR("Failed to load font: %s", name.c_str());
	}

	stbtt_GetFontVMetrics(&this->FontInfo, &this->Ascent, &this->Descent, &this->LineGap);

}
