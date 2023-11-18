#include <PhxEngine/RHI/PhxRHI.h>

#include <PhxEngine/Core/Memory.h>
#include <RHI/DynamicRHIFactory.h>

#include <memory>

using namespace PhxEngine;

namespace
{
	std::unique_ptr<RHI::DynamicRHI> m_dynamicRHI;
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
