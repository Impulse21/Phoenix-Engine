#include "TextureConvertPipeline.h"

#include <DirectXTex.h>

using namespace PhxEngine;

PhxEngine::Pipeline::TextureConvertPipeline::TextureConvertPipeline(IFileSystem* fileSystem)
	: m_fileSystem(fileSystem)
{
}

void PhxEngine::Pipeline::TextureConvertPipeline::Convert(Texture& texture)
{
	if (texture.DataBlob == nullptr)
	{
		// Load Texture Data
		texture.DataBlob = this->m_fileSystem->ReadFile(texture.DataFile);
	}

	if (IBlob::IsEmpty(*texture.DataBlob))
	{
		return;
	}

    DirectX::ScratchImage output;
    DirectX::TexMetadata info;
    bool treatAsLinear = true;
    if (FAILED(DirectX::LoadFromDDSMemory(texture.DataBlob->Data(), texture.DataBlob->Size(), DirectX::DDS_FLAGS_NONE, &info, output)))
    {
        // DDS failed, try WIC
        // Note: try DDS first since WIC can load some DDS (but not all), so we wouldn't want to get 
        // a partial or invalid DDS loaded from WIC.
        if (FAILED(DirectX::LoadFromWICMemory(texture.DataBlob->Data(), texture.DataBlob->Size(), (treatAsLinear ? DirectX::WIC_FLAGS_IGNORE_SRGB : DirectX::WIC_FLAGS_NONE) | DirectX::WIC_FLAGS_FORCE_RGB, &info, output)))
        {
            assert(0 && "Unable to load DDS data");
        }
    }

    // Save to DDS file
    const DirectX::Image* img = output.GetImages();
    size_t nimg = output.GetImageCount();
    HRESULT hr = DirectX::SaveToDDSFile(img, nimg, info, DirectX::DDS_FLAGS_NONE, L"NEW_TEXTURE.DDS");
    assert(SUCCEEDED(hr) && "Failed to save DDS Texture");
}
