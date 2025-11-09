#pragma once

#include "RHICommon.h"

namespace phx::rhi
{
	class IBackend
	{
	public:
		inline static IBackend* Ptr = nullptr;

	public:
		virtual ~IBackend() = default;
		virtual bool Initialize() = 0;
		virtual void Shutdown() = 0;

		// -- Accessors ---
	public:
		virtual ShaderFormat GetShaderFormat() = 0;
		virtual GfxBackend GetBackend() const = 0;

	};
}