#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/StringHash.h>

#define PHX_DECLARE_RESOURCE(TYPE)                                       \
public:                                                                     \
    static constexpr uint64_t StaticTypeHash() { return phx::StringHash(#TYPE); } \
    TYPE() : Resource(StaticTypeHash()) {}

namespace phx
{
	struct Resource : public RefCounted
	{
		const uint64_t resource_type_hash;
		
		enum State : uint8_t
		{
			Loaded = 0,
			Loading = 0x0F,
			Error = 0x7F,
			Unloaded = 0xFF
		};
		std::atomic_uint8_t state = State::Unloaded;

        virtual ~Resource() = default;
		bool IsLoaded()
		{
			return state == State::Loaded;
		}

    protected:
        explicit Resource(uint64_t hash) : resource_type_hash(hash) {}

	};

}