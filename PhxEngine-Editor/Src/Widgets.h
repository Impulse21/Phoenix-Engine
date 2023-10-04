#pragma once

namespace PhxEngine::Editor
{
	class IWidgets
	{
	public:
		virtual ~IWidgets() = default;

		virtual void OnRender() = 0;
	};

}

