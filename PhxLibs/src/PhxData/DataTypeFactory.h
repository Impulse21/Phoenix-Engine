#pragma once

#include <unordered_map>
#include <functional>

#include <PhxCore/StringHash.h>

#define REGISTER_TYPE_FACTORY(TYPE)                         \
    namespace                                               \
    {                                                       \
        struct AutoRegister_##TYPE                          \
        {                                                   \
            AutoRegister_##TYPE()                           \
            {                                               \
                TypeFactory::Register(#TYPE##_hash, []() -> \
                    void* { return new TYPE(); });          \
            }                                               \
        };                                                  \
        static AutoRegister_##TYPE sAutoRegister_##TYPE;    \
    }
namespace phx::data
{
    class DataTypeFactory
    {
    public:
        static void Register(StringHash hash, std::function<void* ()> const& function)
        {
            m_registry.emplace(hash, function);
        }

        template<typename T>
        T* Create()
        {
            return static_cast<T*>(Create(T::TypeId));
        }

        void* Create(StringHash hash)
        {
            auto itr = m_registry.find(hash);
            if (itr == m_registry.end())
                return nullptr;
            return itr->second();
        }

    private:
        inline static std::unordered_map<phx::StringHash, std::function<void*()>> m_registry;
    };
}