#include "PhxRenderer/PhxRenderer_pch.h"
#include "MaterialResourceHandler.h"

#include <PhxRhi/PhxRhi.h>

void phx::renderer::MaterialResourceHandler::LoadAsync(
	IIoQueue* /*io_queue*/,
	RefCountPtr<Resource> /*resource*/,
	AsyncResourceDescriptor const& /*resource_descriptor*/) const
{
}
