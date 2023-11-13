#pragma once

namespace PhxEngine::Core
{
	struct NonCopyable
	{
		NonCopyable() noexcept = default;
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable& operator=(const NonCopyable&) = delete;
	};
}