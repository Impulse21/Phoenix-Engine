#pragma once

#include <PhxEngine/Core/Application.h>
#include "Widgets.h"

namespace PhxEditor 
{
	class IWidget;

	class EditorLayer : public PhxEngine::Layer
	{
	public:
		EditorLayer()
			: Layer("EditorLayer")
		{}

		void OnAttach() override;
		void OnDetach() override;
		void OnImGuiRender() override;

	private:
		template<typename T>
		std::shared_ptr<T> RegisterWidget()
		{
			auto widget = std::make_shared<T>();
			this->m_widgets.emplace_back(widget);
			return widget;
		}

	private:
		bool BeginWindow();

	private:
		std::vector<std::shared_ptr<IWidget>> m_widgets;
	};
}

