#pragma once

#include <PhxEngine/RHI/PhxRHI.h>

namespace PhxEngine
{
	class Renderer final
	{
	public:
		Renderer() = default;
		~Renderer() = default;

		void CreateViewport();
		void ResizeViewport();

		RHI::TextureHandle GetColourBuffer();

	private:
		RHI::TextureHandle m_colourBuffers;
	};
}

