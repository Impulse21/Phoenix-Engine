#pragma once

#include <PhxEngine/Renderer/Renderer.h>

namespace PhxEngine::Renderer
{
	class Dx12Renderer : public Renderer
	{
	public:
		inline static Dx12Renderer* Impl = static_cast<Dx12Renderer*>(Renderer::Ptr);

	public:
	};
}