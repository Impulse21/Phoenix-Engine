#pragma once

#include <PhxEngine/Renderer/RendererSubSystem.h>

namespace PhxEngine
{
	struct WindowResizeEvent;

	class RendererDefaultSubSystem final : public RendererSubSystem
	{
	public:
		~RendererDefaultSubSystem() = default;

		void Startup() override;
		void Shutdown() override;

		void Update() override;
		void Render() override;

	public:
		void OnWindowResize(WindowResizeEvent const& e);

	public:
		DrawableInstanceHandle DrawableInstanceCreate(DrawableInstanceDesc const& desc) override;
		void DrawableInstanceUpdateTransform(DirectX::XMFLOAT4X4 transformMatrix) override;
		void DrawableInstanceOverrideMaterial(MaterialHandle handle) override;
		void DrawableInstanceDelete(DrawableInstanceHandle handle) override;

		void MaterialCreate(MaterialDesc const& desc) override;
		void MaterialUpdate(MaterialHandle handle, MaterialDesc const& desc) override;
		void MaterialDelete(MaterialHandle handle) override;

	private:
		EventHandler<WindowResizeEvent> m_windowResizeHandler;
	};
}

