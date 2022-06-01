#pragma once

#include <string>
#include "Core/TimeStep.h"
#include "Graphics/RHI/PhxRHI.h"

namespace PhxEngine::Core
{
	class ApplicationLayer
	{
	public:
		ApplicationLayer(std::string const& name = "Layer")
			: m_debugName(name)
		{};

		virtual ~ApplicationLayer() = default;

		virtual void OnAttach() {};
		virtual void OnDetach() {};

		virtual void Update(TimeStep const& ts) {};
		virtual void Render() {};
		virtual void Compose(RHI::CommandListHandle cmd) {};

		const std::string& GetName() const { return this->m_debugName; }

	protected:
		std::string m_debugName;
	};
}