#pragma once

#include <PhxEngine/EngineLoop.h>

#ifdef _MSC_VER // Windows
#include <Windows.h>
#define MainFunc int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else // Linux
#define MainFunc int main(int argc, char** argv)
#endif

#define PHX_CREATE_APPLICATION(class_name)		\
    MainFunc									\
    {											\
        class_name app;							\
        PhxEngine::EngineLoop::Run(&app);       \
		return 0;								\
    }