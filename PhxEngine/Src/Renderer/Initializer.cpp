#include "phxpch.h"
#include "PhxEngine/Renderer/Initializer.h"

#include "PhxEngine/Graphics/RHI/PhxRHI.h"

#include "DeferredRenderer.h"

using namespace PhxEngine::RHI;

void PhxEngine::Renderer::Initialize()
{
	// Create Graphics Core
	IRenderer::Ptr = new DeferredRenderer();
	IRenderer::Ptr->Initialize();

}

void PhxEngine::Renderer::Finalize()
{
	if (IRenderer::Ptr)
	{
		IRenderer::Ptr->Finialize();
		delete IRenderer::Ptr;
		IRenderer::Ptr = nullptr;
	};
}