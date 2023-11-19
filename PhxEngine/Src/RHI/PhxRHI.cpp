#include <PhxEngine/RHI/PhxRHI.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Memory.h>
#include <RHI/DynamicRHIFactory.h>

#include <memory>
#include <functional>

using namespace PhxEngine;

using UniqueDynamicRHIPtr = std::unique_ptr<RHI::DynamicRHI, void (*)(RHI::DynamicRHI*)>;
namespace
{
	void CustomDeleter(RHI::DynamicRHI* ptr)
	{
		PHX_LOG_CORE_INFO("Tearing down RHI");
		phx_delete_notnull(ptr);
	}
	UniqueDynamicRHIPtr m_dynamicRHI(nullptr, CustomDeleter);
}

void PhxEngine::RHI::Initialize(RHI::GraphicsAPI api)
{
	assert(!m_dynamicRHI);
	m_dynamicRHI.reset(RHI::DynamicRHIFactory::Create(api));
	m_dynamicRHI->Initialize();
}

void PhxEngine::RHI::Finiailize()
{
	assert(m_dynamicRHI);
	if (m_dynamicRHI)
	{
		m_dynamicRHI->Finalize();
	}
}

RHI::DynamicRHI* PhxEngine::RHI::GetDynamic()
{
	return m_dynamicRHI.get();
}
