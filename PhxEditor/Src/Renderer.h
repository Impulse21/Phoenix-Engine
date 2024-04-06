#pragma once

#include <PhxEngine/Renderer/Renderer.h>

namespace PhxEditor
{
	class Renderer : public PhxEngine::IRenderer
	{
	public:
		Renderer() = default;
		~Renderer() override = default;
	};
}

