#pragma once

#include <atomic>

#include "ResourceTypes.h"

#include <PhxRhi/PhxRhi.h>
#include <PhxCore/RefCountPtr.h>
#include <PhxCore/StringHash.h>

#define PHX_DECLARE_RESOURCE(TYPE)															\
public:																						\
    static constexpr phx::StringHash StaticTypeId() { return phx::StringHash(#TYPE); }	    \
    TYPE() : Resource(StaticTypeId()) {}

namespace phx
{
	struct Resource
	{
		const phx::StringHash type_id;
		std::atomic<unsigned long> ref_counter = 1;
		std::atomic_uint8_t state = ResourceState::Unloaded;


		unsigned long AddRef()
		{
			return ++ref_counter;
		}

		unsigned long Release()
		{
			unsigned long result = --ref_counter;
			if (result == 0)
			{
				Dispose();
				delete this;
			}
			return result;
		}

		virtual void Dispose() = 0;
		virtual bool CollectPendingGpuTransitions(SpanMutable<rhi::GpuBarrier> transitions, size_t& fill_index) = 0;
		virtual RefCountPtr<Resource> GetAliasedResource() { return nullptr; }
		virtual ~Resource() = default;
		bool IsLoaded() { return state = ResourceState::Loaded; }

	protected:
		explicit Resource(phx::StringHash hash) : type_id(hash) {}
	};
}