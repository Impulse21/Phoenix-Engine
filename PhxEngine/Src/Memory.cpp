#include <PhxEngine/Core/Memory.h>

#include <stdlib.h>
#include <assert.h>
#include <stdexcept>

using namespace PhxEngine;

PhxEngine::StackAllocator::StackAllocator(size_t stackSize)
{
    this->m_memory = (uint8_t*)malloc(stackSize);
    this->m_allocatedSize = 0;
    this->m_totalSize = stackSize;
}

PhxEngine::StackAllocator::~StackAllocator()
{
    Free(this->m_memory);
}

void* PhxEngine::StackAllocator::Allocate(size_t size, uint8_t alignment)
{
    assert(size > 0);

    const size_t newStart = MemoryAlign(this->m_allocatedSize, alignment);
    assert(newStart < this->m_totalSize);

    const size_t newAllocatedSize= newStart + size;
    if (newAllocatedSize > m_totalSize) 
    {
        throw std::overflow_error{ "StackAllocator::Allocate => Not enough memory" };
    }

    this->m_allocatedSize = newAllocatedSize;
    return this->m_memory + newStart;
}

void PhxEngine::StackAllocator::Free(void* pointer)
{
    return;
}

PhxEngine::DoubleBufferedAllocator::DoubleBufferedAllocator(size_t stackSize)
    : m_stacks{ StackAllocator{stackSize}, StackAllocator{stackSize} }
    , m_curStackIndex(0) 
{
}

void* PhxEngine::DoubleBufferedAllocator::Allocate(size_t size, uint8_t alignment)
{
    this->m_stacks[this->m_curStackIndex].Allocate(size, alignment);
}

void PhxEngine::DoubleBufferedAllocator::Free(void* pointer)
{
    this->m_stacks[this->m_curStackIndex].Free(pointer);
}

void PhxEngine::DoubleBufferedAllocator::SwapBuffers()
{
    this->m_curStackIndex = !this->m_curStackIndex;
}

void PhxEngine::DoubleBufferedAllocator::ClearCurrentBuffer()
{
    this->m_stacks[this->m_curStackIndex].Clear();
}
