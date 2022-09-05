#pragma once

namespace PhxEngine::Renderer
{
	class Renderer
	{
	public:
		// Global pointer set by intializer
		inline static Renderer* Ptr = nullptr;

	public:
		virtual ~Renderer() = default;
	};
}