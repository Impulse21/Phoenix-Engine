#include "PhxRenderer_pch.h"

#include <PhxRenderer/PhxRenderer.h>

using namespace phx;

void renderer::Initialize(ShaderLibraryDescriptor const& library_desc)
{
	renderer::ShaderLibrary::Ptr = new ShaderLibrary();
	renderer::ShaderLibrary::Ptr->Initialize(library_desc);

}

void renderer::Shutdown()
{
	if (renderer::ShaderLibrary::Ptr)
	{
		renderer::ShaderLibrary::Ptr->Shutdown();
		delete renderer::ShaderLibrary::Ptr;
		renderer::ShaderLibrary::Ptr = nullptr;
	}

}