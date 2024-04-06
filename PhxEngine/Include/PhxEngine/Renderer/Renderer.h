#pragma once

#include <PhxEngine/Core/Math.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Core/Handle.h>

namespace PhxEngine
{
	struct Viewport;
	using ViewportHandle = Handle<Viewport>;

	class IRenderer
	{
	public:
		// Global Pointer to implementation
		inline static IRenderer* Ptr = nullptr;

	public:
		virtual ~IRenderer() = default;

		virtual ViewportHandle CreateViewport() = 0;
		virtual void ResizeViewport(uint32_t width, uint32_t height, ViewportHandle viewport) = 0;

		virtual RHI::TextureHandle GetColourBuffer(ViewportHandle handle) = 0;
	};
}

