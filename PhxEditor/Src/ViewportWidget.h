#pragma once

#include "Widgets.h"

namespace PhxEditor
{
	class ViewportWidget final : public IWidget
	{
	public:
		ViewportWidget() = default;

		void OnImGuiRender();

	private:
	};

	WidgetTitle(ViewportWidget, Viewport);
}

