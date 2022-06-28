#pragma once

#include "Core/TimeStep.h"
#include "Graphics/RHI/PhxRHI.h"

namespace PhxEngine::Graphics
{
	class RenderPathComponent
	{
	public:
		RenderPathComponent() = default;

		virtual void OnAttach() {};
		virtual void OnAttachWindow() {};
		virtual void OnDetach() {};

		virtual void Update(Core::TimeStep const& ts) {};
		virtual void Render() {};
		virtual void Compose(RHI::CommandListHandle cmd) {};
	};
}

