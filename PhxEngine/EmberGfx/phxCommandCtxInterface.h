#pragma once

namespace phx::gfx
{
	class ICommandCtx
	{
	public:
		virtual ~ICommandCtx() = default;

		virtual void RenderPassBegin() = 0;
		virtual void RenderPassEnd() = 0;
	};
}