#include "PhxRenderer/PhxRenderer_pch.h"
#include "DDSTextureCompiler.h"

#include <PhxCore/IO/FileUtils.h>

#include <directxte>
bool phx::renderer::compiler::DSSTextureCompiler::Compile(TextureCompileDescriptor const& desc)
{
	std::string const& file_ext = phx::GetFileExt(desc.virtual_output_path);
	HRESULT hr = S_OK;
	if (file_ext == ".dds" || file_ext == ".DDS")
	{
		hr = LoadFromDDSMemory(
			texBlob.Data(),
			texBlob.Size(),
			DDS_FLAGS_FORCE_RGB,
			&texture->m_metadata,
			texture->m_scratchImage);
	}
	else if (fileExtension == ".hdr" || fileExtension == ".HDR")
	{
		hr = LoadFromHDRMemory(
			texBlob.Data(),
			texBlob.Size(),
			&texture->m_metadata,
			texture->m_scratchImage);
	}
	else if (fileExtension == ".tga" || fileExtension == ".TGA")
	{
		hr = LoadFromTGAMemory(
			texBlob.Data(),
			texBlob.Size(),
			&texture->m_metadata,
			texture->m_scratchImage);
	}
	else
	{
		hr = LoadFromWICMemory(
			texBlob.Data(),
			texBlob.Size(),
			WIC_FLAGS_FORCE_RGB,
			&texture->m_metadata,
			texture->m_scratchImage);
	}
    return true;
}
