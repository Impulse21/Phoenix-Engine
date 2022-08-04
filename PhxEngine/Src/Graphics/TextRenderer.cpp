#include "phxpch.h"
#include "TextRenderer.h"

#include "ThirdParty/TrueTypeFonts/arial.h"

using namespace PhxEngine::Graphics;
// using namespace PhxEngine::RHI;

void PhxEngine::Graphics::TextRenderer::Initialize()
{
	this->m_defaultFontStyle = this->AddFont("arial", arial, sizeof(arial));

	const int borderPadding = 1;
	const float upscaling = 2.0f;
	const float scale = 1.0f;
	const float height = scale * upscaling;

	FontStyle& fontStyle = this->m_loadedFonts[this->m_defaultFontStyle];

	const float fontScaling = stbtt_ScaleForPixelHeight(&fontStyle.FontInfo, height);

	// Add all 128 characters for now, we can optimize later
	for (int c = 0; c < 128; c++)
	{
		this->AddFontGlyph(this->m_defaultFontStyle, c, fontScaling, upscaling, borderPadding);
	}
}

FontHandle PhxEngine::Graphics::TextRenderer::AddFont(std::string const& name, const uint8_t* data, size_t dataSize)
{
	for (size_t i = 0; i < this->m_loadedFonts.size(); i++)
	{
		const FontStyle& fontStyle = this->m_loadedFonts[i];
		if (fontStyle.Name == name)
		{
			return i;
		}
	}

	this->m_loadedFonts.emplace_back();
	this->m_loadedFonts.back().Create(name, data, dataSize);
	return this->m_loadedFonts.size() -1;
}

void PhxEngine::Graphics::TextRenderer::AddFontGlyph(
	FontHandle fontHandle,
	int code,
	float fontScaling,
	float upscaling,
	int boarderPadding)
{
	FontStyle& fontStyle = this->m_loadedFonts[fontHandle];

	// Font resolution is upscaled to make it sharper:


	int left, top, right, bottom;
	stbtt_GetCodepointBitmapBox(
		&fontStyle.FontInfo,
		code,
		fontScaling,
		fontScaling, 
		&left,
		&top,
		&right,
		&bottom);

	Glyph& glyph = this->m_glyphLookup[code];
	glyph.X = static_cast<float>(left);
	glyph.Y = static_cast<float>(top) + static_cast<float>(fontStyle.Ascent) * fontScaling;
	glyph.Width = static_cast<float>(right - left);
	glyph.Height = static_cast<float>(bottom - top);

	// Remove dpi upscaling:
	glyph.X = glyph.X / upscaling;
	glyph.Y = glyph.Y / upscaling;
	glyph.Width = glyph.Width / upscaling;
	glyph.Height = glyph.Height / upscaling;

	// Add padding to the rectangle that will be packed in the atlas:
	right += boarderPadding * 2;
	bottom += boarderPadding * 2;
	// TODO: Border Lookup

}

void TextRenderer::FontStyle::Create(std::string const& name, const uint8_t* data, size_t size)
{
	this->Name = name;
	int offset = stbtt_GetFontOffsetForIndex(data, 0);
	
	const bool initFont = stbtt_InitFont(&this->FontInfo, data, offset);
	assert(initFont);

	stbtt_GetFontVMetrics(&this->FontInfo, &this->Ascent, &this->Descent, &this->LineGap);



}