#include <PhxEngine/Renderer/FontRenderer.h>

#include <PhxEngine/Core/FileSystem.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/BinaryReader.h>

using namespace PhxEngine::Renderer;
using namespace PhxEngine::Core;

static const char spriteFontMagic[] = "DXTKfont";


/*
FontRenderer(
	std::string const& fontFilename,
	std::shared_ptr<Core::IFileSystem> fileSystem,
	RHI::IGraphicDevice& graphicsDevice)
	: m_graphicsDevice(graphicsDevice)
	, m_fs(fileSystem)
{
	this->LoadDXTKFontFile(filename);
}

void FontRenderer::LoadFontFile(std::string const& fontFileName)
{
	if (!this->m_fs->FileExists(fontFileName))
	{
		LOG_CORE_FATAL("Failed to locate engine font file %s", fontFileName.c_str());
		throw std::runtime_error("Unable to locate engine font file");
	}

	auto blob = this->m_fs->ReadFile(fontFileName);
	BinaryReader reader(std::move(blob));

}
*/
