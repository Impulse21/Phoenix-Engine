#pragma once

#ifdef _DEBUG
#include <optick.h>

#define PHX_FRAME(FRAME_NAME, ...)	OPTICK_FRAME(FRAME_NAME, __VA_ARGS__)
#define PHX_EVENT(...)				OPTICK_EVENT(__VA_ARGS__)
#define PHX_THREAD(THREAD_NAME)		OPTICK_THREAD(THREAD_NAME)

#else

#define PHX_FRAME(FRAME_NAME, ...)
#define PHX_EVENT(...)			
#define PHX_THREAD(THREAD_NAME)	

#endif