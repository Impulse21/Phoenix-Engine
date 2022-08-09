#pragma once

#include <string>
#include "PhxEngine/Core/TimeStep.h"

namespace PhxEngine
{
	class AppLayer
	{
	public:
		AppLayer(std::string const& name = "Layer")
			: m_debugName(name)
		{};

		virtual ~AppLayer() = default;

		virtual void OnAttach() {};
		virtual void OnDetach() {};

		virtual void OnUpdate(Core::TimeStep const& ts) {};
		virtual void OnRenderImGui() {};

		const std::string& GetName() const { return this->m_debugName; }

	protected:
		std::string m_debugName;
	};
}