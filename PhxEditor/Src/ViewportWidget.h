#pragma once

#include "Widgets.h"
#include <PhxEngine/Renderer/Renderer.h>

namespace PhxEditor
{
	class ViewportWidget final : public IWidget
	{
	public:
		ViewportWidget() = default;

		void OnImGuiRender();

	private:
		PhxEngine::ViewportHandle m_viewportHandle;
	};

	WidgetTitle(ViewportWidget, Viewport);
}

