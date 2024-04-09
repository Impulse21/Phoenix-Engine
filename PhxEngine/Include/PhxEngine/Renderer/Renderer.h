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

		virtual ViewportHandle ViewportCreate() = 0;
		virtual void ViewportResize(uint32_t width, uint32_t height, ViewportHandle viewport) = 0;
		virtual DirectX::XMFLOAT2 ViewportGetSize(ViewportHandle viewport) = 0;
		virtual RHI::TextureHandle ViewportGetColourBuffer(ViewportHandle handle) = 0;

		virtual void OnUpdate() = 0;

		virtual void LoadShaders() = 0;
		virtual PhxEngine::Span<RHI::ShaderHandle> GetShaderList() const = 0;

	};
}

