#pragma once

#include "phxGfxCommandCtxSelector.h"

namespace phx::gfx
{
#ifdef PHX_VIRTUAL_DEVICE
	using CommandList = ICommandList;
#else
	using CommandList = GfxCommandSelector<kSelectedAPI>::CommandListType;
#endif
}