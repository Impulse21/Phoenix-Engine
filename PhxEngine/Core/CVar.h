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

    template<>
    struct CVarTypeResolver<bool>{ static constexpr CVarType Type = CVarType::Bool; }
    template<>
    struct CVarTypeResolver<int>{ static constexpr CVarType Type = CVarType::Int; }
    template<>
    struct CVarTypeResolver<float>{ static constexpr CVarType Type = CVarType::Float; }
    template<>
    struct CVarTypeResolver<std::string>{ static constexpr CVarType Type = CVarType::String; }

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
        static void ForEach(TFn& fn)
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
    };
}

#define PHX_CVAR_FLOAT(name, desc, default)     \
	phx::CTypedVar<float> CVar_##name(#name, desc, default)

#define PHX_CVAR_INT(name, desc, default)       \
	phx::CTypedVar<int> CVar_##name(#name, desc, default)

#define PHX_CVAR_BOOL(name, desc, default)      \
	phx::CTypedVar<bool> CVar_##name(#name, desc, default)

#define PHX_CVAR_STRING(name, desc, default)    \
	phx::CTypedVar<std::string> CVar_##name(#name, desc, default)


#define PHX_CVAR_FLOAT(name)     \
	extern phx::CTypedVar<float> CVar_##name

#define PHX_CVAR_INT(name)       \
	extern phx::CTypedVar<int> CVar_##name

#define PHX_CVAR_BOOL(name)      \
	extern phx::CTypedVar<bool> CVar_##name

#define PHX_CVAR_STRING(name)    \
	extern phx::CTypedVar<std::string> CVar_##name