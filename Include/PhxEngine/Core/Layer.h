#pragma once

#include <string>
#include <PhxEngine/Core/TimeStep.h>
#include <PhxEngine/RHI/PhxRHI.h>

namespace PhxEngine::Core
{
	class Layer
	{
	public:
		Layer(std::string const& name = "Layer")
			: m_debugName(name)
		{};

		virtual ~Layer() = default;

		virtual void OnAttach() {};
		virtual void OnDetach() {};
		virtual void OnUpdate(TimeStep const& ts) {};
		virtual void OnRender(RHI::CommandListHandle cmdList) {};

		const std::string& GetName() const { return this->m_debugName; }

	protected:
		std::string m_debugName;
	};
}