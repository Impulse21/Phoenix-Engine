#pragma once

#ifdef _WIN32
#include <wtypes.h>
#endif


namespace phx::core
{
#ifdef _WIN32
	using WindowHandle = HWND;
#endif
}