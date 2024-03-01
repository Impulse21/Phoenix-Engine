#include <PhxEngine/EngineMemory.h>

#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/EngineTuner.h>


using namespace PhxEngine;

SizeVar ServiceAndLevelStackAllocatorSize("Engine/Memroy/Services And Level Stack Allocator Size", PhxMB(10));
SizeVar SingleFrameAllocatorSize("Engine/Memroy/Single Frame Allocator Size", PhxMB(10));
SizeVar DoubleBufferAllocatorSize("Engine/Memroy/Double Buffer Allocator Size", PhxMB(10));

void PhxEngine::EngineMemory::Startup()
{
}

void PhxEngine::EngineMemory::Shutdown()
{
}

void PhxEngine::EngineMemory::Update()
{
}
