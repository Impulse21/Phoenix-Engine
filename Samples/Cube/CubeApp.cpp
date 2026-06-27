#include "CubeApp.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Memory/TlsfHeapAllocator.h>
#include <PhxEngine/Memory/MemoryHelpers.h>

#include <PhxEngine/Renderer/RenderGraph.h>
#include <PhxEngine/RHI/RHI.h>

#include <PhxEngine/Platform/EntryPoint.h>
#include <PhxEngine/Engine.h>

using namespace samples;
using namespace phx;

PHX_DEFINE_APP(CubeApp);

const char* samples::CubeApp::GetName() const { return "PhxCubeApp"; }

void samples::CubeApp::OnInit() 
{
}

void samples::CubeApp::OnBuildGraph(renderer::RenderGraphBuilder& rg_builder, RenderWorld& world) 
{
    PHX_UNUSED(rg_builder);
    PHX_UNUSED(world);
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
