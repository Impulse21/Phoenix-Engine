#include "CVar.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>

using namespace phx;

namespace 
{
    phx::CVar* FindCVar(const char* name)
    {
        for (phx::CVar* curr = CVar::s_head; curr; curr = curr->next)
        {
            if (std::strcmp(curr->name, name) == 0)
                return curr;
        }

        return nullptr;
    }

    void ApplyValue(CVar* cvar, const char* value)
    {
        switch (cvar->type)
        {
            case phx::CVarType::Int:
                static_cast<phx::CVarTyped<int>*>(cvar)->Set(atoi(value));
                break;

            case phx::CVarType::Float:
                static_cast<phx::CVarTyped<float>*>(cvar)->Set((float)atof(value));
                break;

            case phx::CVarType::Bool:
                static_cast<phx::CVarTyped<bool>*>(cvar)->Set(
                    strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
                break;

            case phx::CVarType::String:
                static_cast<phx::CVarTyped<const char*>*>(cvar)->Set(value);
                break;
        }
    }
}

void CVar::Initialize(Span<char*> args)
{
    LoadConfig("phx.cfg");
    for (size_t i = 0; i < args.size(); ++i)
    {
        const char* arg = args[i];

        if (std::strncmp(arg, "-", 1) != 0)
            continue;

        const char* eq = strchr(arg, '=');

        if (eq)
        {
            char name[128] = {};
            size_t name_len = (size_t)(eq - arg);

            name_len = name_len < sizeof(name) - 1 ? name_len : sizeof(name) - 1;

            strncpy(name, arg, name_len);
            name[name_len] = '\0';
            
            CVar* cvar = FindCVar(name + 1);
            if (cvar) 
                ApplyValue(cvar, eq + 1);   
        }
        else
        {
            // Foramt: -flags
            CVar* cvar = FindCVar(arg + 1);
            if (cvar && cvar->type == CVarType::Bool)
                static_cast<CVarTyped<bool>*>(cvar)->Set(true);
        }
    }
}

void CVar::Shutdown()
{

}

void CVar::LoadConfig(const char* /*file*/)
{
    // no-op
}