#pragma once

#include <PhxEngine/Core/StringHash.h>

#include <atomic>

#define PHX_DECLARE_RESOURCE(TYPE)															\
public:																						\
    static constexpr phx::StringHash StaticTypeId() { return phx::StringHash(#TYPE); }	    \
    TYPE() : Resource(StaticTypeId()) {}


namespace phx::resources
{
    struct Resource
    {
        enum State : uint8_t
        {
            Unloaded = 0,

            // AsyncLoader has started.
            // Loaders can use [0x02 - 0x1F] for internal steps (e.g. Loading +
            // 1)
            Loading = 0x01,

            Waiting_dependencies = 0x20,
            Copied_to_gpu = 0x40,
            Pending_gfx_transition = 0x50,
            Loaded = 0x60,

            Error = 0xFF
        };

        const phx::StringHash type_id;
        std::atomic_uint32_t ref_counter = 1;
        std::atomic_uint8_t state = State::Unloaded;



		virtual void Dispose() {};

		uint32_t AddRef()
		{
			return ++ref_counter;
		}

		uint32_t Release()
		{
			uint32_t result = --ref_counter;
			if (result == 0)
			{
				Dispose();
				delete this;
			}

			return result;
		}

        bool IsLoaded() const { return state == State::Loaded; }

        virtual ~Resource() = default;

	protected:
		explicit Resource(phx::StringHash hash) : type_id(hash) {}
    };
}