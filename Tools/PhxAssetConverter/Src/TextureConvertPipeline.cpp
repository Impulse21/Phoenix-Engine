#include "TextureConvertPipeline.h"

#include <DirectXTex.h>

using namespace PhxEngine;

PhxEngine::Pipeline::TextureConvertPipeline::TextureConvertPipeline(Core::IFileSystem* fileSystem)
	: m_fileSystem(fileSystem)
{
}

void PhxEngine::Pipeline::TextureConvertPipeline::Convert(Texture& texture, TexConversionFlags additionalFlags)
{
	texture.DDSImage = texture.DataBlob == nullptr 
		? DDSTextureConverter::BuildDDS(this->m_fileSystem, texture.DataFile, (additionalFlags | texture.ConvertFlags))
		: DDSTextureConverter::BuildDDS(texture.DataBlob.get(), GetTextureType(texture.DataFile), additionalFlags | texture.ConvertFlags);

	texture.DataBlob.reset();
}
