#include "PhxRenderer/PhxRenderer_pch.h"
#include "TextureResourceHandler.h"

#include <PhxRhi/PhxRhi.h>

void phx::renderer::TextureResourceHandler::LoadAsync(
	IIoQueue* /*io_queue*/,
	RefCountPtr<Resource> /*resource*/,
	AsyncResourceDescriptor const& /*resource_descriptor*/) const
{
}
