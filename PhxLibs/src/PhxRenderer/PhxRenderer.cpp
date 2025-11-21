#include "PhxRenderer/PhxRenderer_pch.h"
#include "PhxRenderer.h"

#include "PhxRenderer.h"

#include <PhxRhi/IBackend.h>

#include <PhxRenderer/ShaderLibrary.h>

void phx::renderer::Initialize(rhi::IBackend* rhi_backend)
{
	ShaderLibrary::Ptr = new ShaderLibrary();
	ShaderLibraryDescriptor shader_libraryDesc = {
		.target = rhi_backend->GetShaderFormat(),
	};
}

void phx::renderer::Shutdown()
{
	if (ShaderLibrary::Ptr)
	{
		ShaderLibrary::Ptr->Shutdown();
		delete ShaderLibrary::Ptr;
		ShaderLibrary::Ptr = nullptr;
	}

}

