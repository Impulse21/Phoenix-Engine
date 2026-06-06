#include "CubeApp.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Memory/TlsfHeapAllocator.h>
#include <PhxEngine/Memory/MemoryHelpers.h>

#include <PhxEngine/Platform/EntryPoint.h>
#include <PhxEngine/Engine.h>

using namespace samples;

PHX_DEFINE_APP(CubeApp);

const char* samples::CubeApp::GetName() const { return "PhxCubeApp"; }

void samples::CubeApp::OnInit() 
{
}

void samples::CubeApp::OnFillWorld(phx::RenderWorld& /*world*/) 
{
}

void samples::CubeApp::OnUpdate(float /*dt*/) 
{

}

void samples::CubeApp::OnRender() 
{

}

void samples::CubeApp::OnShutdown() 
{

}
