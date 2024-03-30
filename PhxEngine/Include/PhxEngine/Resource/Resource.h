#pragma once

#include <PhxEngine/Core/RefCountPtr.h>
#include <atomic>

namespace PhxEngine
{
	namespace ResourceStatus
	{
		constexpr uint8_t Loaded = 0;
		constexpr uint8_t Loading = 0xFD;
		constexpr uint8_t Unloaded = 0xFE;
		constexpr uint8_t Failed = 0xFF;
	}

	class Resource : public RefCounted
	{
		PHX_OBJECT(Resource, RefCounted);

	public:
		Resource() = default;
		virtual ~Resource() = default;

	protected:
		std::atomic_uint8_t m_state;
	};

	template<typename T>
	class ResourceRegistry : public Object
	{
		PHX_OBJECT(ResourceRegistry, Object);

	protected:
		ResourceRegistry() = default;
		virtual ~ResourceRegistry() = default;

		virtual RefCountPtr<T> Retrieve(StringHash meshname) = 0;

	protected:
		std::unordered_map<StringHash, RefCountPtr<T>> m_registry;

	};
}

