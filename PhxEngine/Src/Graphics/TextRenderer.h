#pragma once

#include <unordered_map>
#include <vector>

#include "ThirdParty/stb_truetype.h"

namespace PhxEngine
{
	namespace Graphics
	{
		using FontHandle = size_t;

		class TextRenderer
		{
		public:
			TextRenderer() = default;
			~TextRenderer() = default;

			void Initialize();
			void DrawString(std::string const& text);
			void DrawString(const char* text);

		private:
			FontHandle AddFont(std::string const& name, const uint8_t* data, size_t dataSize);
			void AddFontGlyph(
				FontHandle fontHandle,
				int code,
				float fontScaling,
				float upscaling,
				int boarderPadding);

		private:
			// https://learnopengl.com/In-Practice/Text-Rendering
			struct Glyph
			{
				float X;
				float Y;
				float Width;
				float Height;
				float TexCoordLeft;
				float TexCoordRight;
				float TexCoordTop;
				float TexCoordBottom;
			};

			struct FontStyle
			{
				std::string Name;
				stbtt_fontinfo FontInfo;
				int Ascent;
				int Descent;
				int LineGap;
				void Create(std::string const& name, const uint8_t* data, size_t size);
			};

		private:
			std::vector<FontStyle> m_loadedFonts;
			std::unordered_map<char, Glyph> m_glyphLookup;
			FontHandle m_defaultFontStyle;
		};
	}
}