#pragma once

namespace PhxEngine::Core
{
	struct NonMoveable
	{
		NonMoveable() = default;
		NonMoveable(NonMoveable&&) = delete;
		NonMoveable& operator=(NonMoveable&&) = delete;
	};
}