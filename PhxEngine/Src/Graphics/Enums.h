#pragma once

namespace PhxEngine::Graphics
{
	enum class BlendMode
	{
		Opaque = 0,
		Alpha,
		Count
	};

	enum RenderType
	{
		RenderType_Void = 0,
		RenderType_Opaque = 1 << 0,
		RenderType_Transparent = 1 << 1,
		RenderType_All = RenderType_Opaque | RenderType_Transparent
	};
}