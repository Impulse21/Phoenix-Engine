#include "phxpch.h"
#include "PhxEngine/Renderer/Initializer.h"

#include "PhxEngine/RHI/PhxRHI.h"

#include "DeferredRenderer.h"

using namespace PhxEngine::RHI;

void PhxEngine::Renderer::Initialize()
{
	// Create Graphics Core
	IRenderer::GPtr = new DeferredRenderer();
	IRenderer::GPtr->Initialize();

}

void PhxEngine::Renderer::Finalize()
{
	if (IRenderer::GPtr)
	{
		IRenderer::GPtr->Finialize();
		delete IRenderer::GPtr;
		IRenderer::GPtr = nullptr;
	};
}