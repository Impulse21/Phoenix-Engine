#include "phxpch.h"
#include "PhxEngine/Renderer/Initializer.h"

#include "PhxEngine/Graphics/RHI/PhxRHI.h"

#include "Dx12/Dx12Renderer.h"
#include "Dx12/Dx12ResourceManager.h"

using namespace PhxEngine::RHI;

void PhxEngine::Renderer::Initialize()
{
	// Create Graphics Core
	ResourceManager::Ptr = new Dx12ResourceManager();
	Renderer::Ptr = new Dx12Renderer();

}

void PhxEngine::Renderer::Finalize()
{
	if (Renderer::Ptr)
	{
		delete Renderer::Ptr;
		Renderer::Ptr = nullptr;
	};

	if (ResourceManager::Ptr)
	{
		delete ResourceManager::Ptr;
		ResourceManager::Ptr = nullptr;
	};
}