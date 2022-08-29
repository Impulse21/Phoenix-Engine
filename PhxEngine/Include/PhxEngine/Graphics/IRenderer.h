#pragma once

#include "RHI/PhxRHI.h"

namespace PhxEngine::Scene
{
	namespace New
	{
		struct CameraComponent;
		struct Scene;
	}
}
namespace PhxEngine::Graphics
{
	class IRenderer
	{
	public:
		virtual ~IRenderer() = default;

		virtual void Initialize() = 0;
		virtual void Finialize() = 0;

		virtual void RenderScene(Scene::New::CameraComponent const& camera, Scene::New::Scene const& scene) = 0;

		virtual RHI::TextureHandle GetFinalColourBuffer() = 0;

		virtual void OnWindowResize(DirectX::XMFLOAT2 const& size) = 0;
	};
}