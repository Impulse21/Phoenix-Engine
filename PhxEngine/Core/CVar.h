#pragma once

#include <PhxEngine/Core/Span.h>

namespace phx
{
    enum class CVarType
    {
        Bool = 0,
        Int,
        Float,
        String
    };

    template<typename T>
    struct CVarTypeResolver;

    template<> struct CVarTypeResolver<bool>{ static constexpr CVarType Type = CVarType::Bool; };
    template<> struct CVarTypeResolver<int>{ static constexpr CVarType Type = CVarType::Int; };
    template<> struct CVarTypeResolver<float>{ static constexpr CVarType Type = CVarType::Float; };
    template<> struct CVarTypeResolver<const char*>{ static constexpr CVarType Type = CVarType::String; };

    struct CVar
    {
        const char* name;
        const char* description;
        CVarType type;
        CVar* next = nullptr;

    // public interface
        inline static CVar* s_head;

        // TODO: Impl
        static void Initialize(Span<char*> args);
        static void Shutdown();
        static void LoadConfig(const char* file);

        template<typename TFn>
        static void ForEach(TFn fn)
        {
            for (CVar* curr = s_head; curr; curr = curr->next)
                fn(curr);
        }
        
    protected:
        CVar(const char* name, const char* desc, CVarType type)
            : name(name)
            , description(desc)
            , type(type)
        {
            next = s_head;
            s_head = this;
        }
    };

    template<typename T>
    struct CVarTyped : public CVar
    {
        T value;
        T default_value;
        
        CVarTyped(const char* name, const char* desc, T default_value)
            : CVar(name, desc, CVarTypeResolver<T>::Type)
            , value(default_value)
            , default_value(default_value) {};


        [[nodiscard]] T   Get  () const { return value;         }
        void              Set  (T v)    { value = v;            }
        void              Reset()       { value = default_value; }
    };
}

#define PHX_CVAR_FLOAT(name, default, desc)     \
	phx::CVarTyped<float> CVar_##name(#name, desc, default)

#define PHX_CVAR_INT(name, default, desc)       \
	phx::CVarTyped<int> CVar_##name(#name, desc, default)

#define PHX_CVAR_BOOL(name, default, desc)      \
	phx::CVarTyped<bool> CVar_##name(#name, desc, default)

#define PHX_CVAR_STRING(name, default, desc)    \
	phx::CVarTyped<const char*> CVar_##name(#name, desc, default)


#define PHX_XCVAR_FLOAT(name)     \
	extern phx::CVarTyped<float> CVar_##name

#define PHX_XCVAR_INT(name)       \
	extern phx::CVarTyped<int> CVar_##name

#define PHX_XCVAR_BOOL(name)      \
	extern phx::CVarTyped<bool> CVar_##name

#define PHX_XCVAR_STRING(name)    \
	extern phx::CVarTyped<const char*> CVar_##name