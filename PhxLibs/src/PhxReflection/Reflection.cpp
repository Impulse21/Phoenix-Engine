#include "PhxReflection_pch.h"

#include "Reflection.h"

#include <unordered_map>
#include <string_view>

using namespace phx::reflection;

namespace
{
	class ReflectionRegistryImpl : public phx::reflection::IReflectionRegistry
	{
	public:
		ReflectionRegistryImpl() = default;

	public:
		void RegisterType(const TypeInfo* typeInfo) override
		{
			if (typeInfo && typeInfo->name)
			{
				m_typeMap[typeInfo->name] = typeInfo;
			}
		};

		const TypeInfo* FindType(const char* name) const override
		{
			auto it = m_typeMap.find(name);
			if (it != m_typeMap.end())
			{
				return it->second;
			}

			return nullptr;
		};

	private:
		ReflectionRegistryImpl(const ReflectionRegistryImpl&) = delete;
		ReflectionRegistryImpl& operator=(const ReflectionRegistryImpl&) = delete;

	private:
		std::unordered_map<std::string_view, const TypeInfo*> m_typeMap;
	};
}

void phx::reflection::Initialize()
{
	IReflectionRegistry::Ptr = new ReflectionRegistryImpl();

	// Reflect Core Types

}

void phx::reflection::Shutdown()
{
	if (IReflectionRegistry::Ptr)
		delete IReflectionRegistry::Ptr;
}