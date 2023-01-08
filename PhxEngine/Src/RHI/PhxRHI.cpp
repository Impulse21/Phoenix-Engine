#include <phxpch.h>

#include <PhxEngine/RHI/PhxRHI.h>

using namespace PhxEngine::RHI;

// Global
IRHI* PhxEngine::RHI::GRHI = nullptr;

void RHIInitialize()
{
	if (GRHI)
	{
		LOG_CORE_WARN("RHI Already Initialized");
		return;
	}

	// Call Platform specific iniitalization
	LOG_CORE_WARN("Initializing RHI");

	std::unique_ptr<IRHI> safeGRHIPtr = PlatformCreateRHI();

	// Take full control of the pointer for now, should be made safier but I want explicit control.
	GRHI = safeGRHIPtr.release();

	if (!GRHI)
	{
		LOG_CORE_FATAL("Failed to create RHI");
		throw std::runtime_error("Failed to create RHI");
	}

	// To what ever else is required
}

void RHIFinalize()
{
	if (!GRHI)
	{
		LOG_CORE_WARN("RHI was never initialized.");
		return;
	}

	delete GRHI;
	GRHI = nullptr;
}