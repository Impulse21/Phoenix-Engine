#pragma once

#include <string>
#include <memory>

namespace PhxEngine
{
	namespace Core
	{
		class IFileSystem;
		class BinaryReader;
	}

	namespace RHI
	{
		class IGraphicDevice;
	}

	namespace Renderer
	{
		class FontRenderer
		{
		public:
			FontRenderer(
				std::string const& fontFilename,
				RHI::IGraphicDevice& graphicsDevice);
		};
		/*
		class FontRenderer
		{
		public:
			FontRenderer(
				std::string const& fontFilename,
				std::shared_ptr<Core::IFileSystem> fileSystem,
				RHI::IGraphicDevice& graphicsDevice);

			~FontRenderer() = default;

		private:
			void LoadFontFile(std::string const& fontFileName);
			void LoadDXTKFontFile(BinaryReader& reader);

		private:
			RHI::IGraphicDevice& m_graphicsDevice;
			std::shared_ptr<IFileSystem> m_fs;
		};
		*/
	}
}

