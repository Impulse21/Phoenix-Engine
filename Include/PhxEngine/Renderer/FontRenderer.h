#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include <Shaders/ShaderInteropStructures.h>
#include <PhxEngine/Utility/stb_truetype.h>
#include <PhxEngine/Core/Math.h>
#include <PhxEngine/RHI/PhxRHI.h>

namespace Shader
{
	struct FontVertex;
}

namespace PhxEngine
{
#define WHITESPACE_SIZE ((float(params.Size) + params.SpacingX) * params.Scaling * 0.25f)
#define TAB_SIZE (WHITESPACE_SIZE * 4)
#define LINEBREAK_SIZE ((float(params.Size) + params.SpacingY) * params.Scaling)

	namespace Core
	{
		class IFileSystem;
		class BinaryReader;
	}
	static constexpr int sDefaultFontSize = 16;

	struct DrawParams
	{
		float PosX = 0;
		float PosY = 0;
		int Size = sDefaultFontSize;
		float Scaling = 1;
		float SpacingX = 1; // minimum spacing between characters
		float SpacingY = 1; // minimum spacing between characters
		const int Style = 0; // Only support a single font at the moment.
		float DisplaySizeX = 0;
		float DisplaySizeY = 0;
		RHI::Color Color = RHI::Color(1.0f); // Default white
	};

	namespace Renderer
	{
		class FontRenderer
		{
		public:
			FontRenderer(RHI::IGraphicsDevice* graphicsDevice);

			void DrawString(std::string const& text, DrawParams const& params, RHI::CommandListHandle cmdList);
			void DrawString(std::wstring const& text, DrawParams const& params, RHI::CommandListHandle cmdList);
			void DrawString(const char* text, DrawParams const& params, RHI::CommandListHandle cmdList);
			void DrawString(const wchar_t* text, DrawParams const& params, RHI::CommandListHandle cmdList);

		private:
			struct Glyph
			{
				float X;
				float Y;
				float Width;
				float Height;
				float Tc_left;
				float Tc_right;
				float Tc_top;
				float Tc_bottom;
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

			template<typename T>
			void DrawStringInternal(const T* text, size_t textLength, DrawParams const& params, RHI::CommandListHandle cmdList)
			{
				if (textLength == 0)
				{
					return;
				}

				// Write Vertex Data
				static thread_local std::vector<Shader::FontVertex> textbuffer;
				textbuffer.resize(textLength * 4);
				const uint32_t quadCount = this->WriteVertices(textbuffer.data(), text, params);

				if (quadCount > 0)
				{
					auto& _scropedMarker = cmdList->BeginScropedMarker("Font");
					cmdList->SetGraphicsPSO(this->m_fontPso);
					
					Shader::FontDrawInfo drawInfo = {};
					drawInfo.ColorPacked = Math::PackColour(params.Color);
					drawInfo.TextureIndex = this->m_fontTexture->GetDescriptorIndex();

					float L = params.PosX;
					float R = params.PosX + params.DisplaySizeX;
					float T = params.PosY;
					float B = params.PosY + params.DisplaySizeY;
					static const float mvp[4][4] =
					{
						{ 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
						{ 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
						{ 0.0f,         0.0f,           0.5f,       0.0f },
						{ (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
					};

					drawInfo.Mvp = DirectX::XMFLOAT4X4(&mvp[0][0]);

					cmdList->BindPushConstant(0, drawInfo);

					cmdList->BindDynamicVertexBuffer(
						0,
						textbuffer.size(),
						sizeof(Shader::FontVertex),
						textbuffer.data());

					cmdList->Draw(4, quadCount);
				}

				this->UpdatePendingGlyphs();
			}

			template<typename T>
			uint32_t WriteVertices(Shader::FontVertex* vertexList, const T* text, DrawParams const& params)
			{
				auto& fontStyle = this->m_fontStyles[params.Style];
				const float fontScale = stbtt_ScaleForPixelHeight(&fontStyle.FontInfo, (float)params.Size);

				uint32_t quadCount = 0;
				float line = 0;
				float pos;

				int codePrev = 0;
				size_t i = 0;
				while (text[i] != 0)
				{
					T character = text[i++];
					int code = (int)character;
					const int32_t hash = this->CreateGlyphHash(code, params.Style, params.Size);

					if (this->m_glyphLookup.count(hash) == 0)
					{
						this->m_pendingGlyphs.insert(hash);
						continue;
					}

					switch (code)
					{
					case '\n':
						line += LINEBREAK_SIZE;
						pos = 0;
						codePrev = 0;
						break;

					case ' ':
						pos += WHITESPACE_SIZE;
						codePrev = 0;
						break;

					case '\t':
						pos += TAB_SIZE;
						codePrev = 0;
						break;

					default:
						const Glyph& glyph = this->m_glyphLookup.at(hash);
						const float glyphWidth = glyph.Width * params.Scaling;
						const float glyphHeight = glyph.Height * params.Scaling;
						const float glyphOffsetX = glyph.X * params.Scaling;
						const float glyphOffsetY = glyph.Y * params.Scaling;

						const size_t vertexID = size_t(quadCount) * 4;

						if (codePrev != 0)
						{
							int kern = stbtt_GetCodepointKernAdvance(&fontStyle.FontInfo, codePrev, code);
							pos += kern * fontScale;
						}
						codePrev = code;

						const float left = pos + glyphOffsetX;
						const float right = left + glyphWidth;
						const float top = line + glyphOffsetY;
						const float bottom = top + glyphHeight;

						vertexList[vertexID + 0].Position = float2(left, top);
						vertexList[vertexID + 1].Position = float2(right, top);
						vertexList[vertexID + 2].Position = float2(left, bottom);
						vertexList[vertexID + 3].Position = float2(right, bottom);

						vertexList[vertexID + 0].TexCoord = float2(glyph.Tc_left, glyph.Tc_top);
						vertexList[vertexID + 1].TexCoord = float2(glyph.Tc_right, glyph.Tc_top);
						vertexList[vertexID + 2].TexCoord = float2(glyph.Tc_left, glyph.Tc_bottom);
						vertexList[vertexID + 3].TexCoord = float2(glyph.Tc_right, glyph.Tc_bottom);

						pos += glyph.Width * params.Scaling + params.SpacingX;
						quadCount++;
					}
				}

				return quadCount;
			}
			
			size_t AddFontStyle(std::string const& name, const uint8_t* data, size_t size);

			void UpdatePendingGlyphs();

		private:
			constexpr int32_t CreateGlyphHash(int code, int style, int height) 
			{ 
				return ((code & 0xFFFF) << 16) | ((style & 0x3F) << 10) | (height & 0x3FF);
			}
			constexpr int GetCodeFromHash(int64_t hash) { return int((hash >> 16) & 0xFFFF); }
			constexpr int GetStyleFromHash(int64_t hash) { return int((hash >> 10) & 0x3F); }
			constexpr int GetHeightFromHash(int64_t hash) { return int((hash >> 0) & 0x3FF); }

		private:
			void LoadShaderData();

		private:
			RHI::IGraphicsDevice* m_graphicsDevice;
			std::shared_ptr<Core::IFileSystem> m_fileSystem;
			std::vector<FontStyle> m_fontStyles;
			std::unordered_map<int32_t, Glyph> m_glyphLookup;
			std::unordered_set<int32_t> m_pendingGlyphs;
			std::unordered_map<int32_t, RHI::Rect> m_rectLookup;

			RHI::TextureHandle m_fontTexture;
			RHI::GraphicsPSOHandle m_fontPso;
		};

	}
}

