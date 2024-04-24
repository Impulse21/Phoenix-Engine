#include <PhxEngine/Core/CommandLineArgs.h>

#include <unordered_map>

#include <stdlib.h>
#include <cstdlib>

namespace
{
    std::unordered_map<std::string, std::string> m_arguments;

    template<typename FunctionType>
    bool Lookup(std::string const& key, const FunctionType& pfnFunction)
    {
        const auto found = m_arguments.find(key);
        if (found != m_arguments.end())
        {
            pfnFunction(found->second);
            return true;
        }
        return false;
    }


    template<typename GetStringFunc>
    void GatherArgs(size_t numArgs, const GetStringFunc& pfnGet)
    {
        // the first arg is always the exe name and we are looing for key/value pairs
        if (numArgs > 1)
        {
            size_t i = 0;

            while (i < numArgs)
            {
                const char* key = pfnGet(i);
                i++;
                if (i < numArgs && key[0] == '-')
                {
                    const char* strippedKey = key + 1;
                    m_arguments[strippedKey] = pfnGet(i);
                    i++;
                }
            }
        }
    }
}

void PhxEngine::CommandLineArgs::Initialize()
{
    GatherArgs(__argc, [](size_t i) -> const char* {
        return __argv[i];
        });
}

bool PhxEngine::CommandLineArgs::HasArg(std::string const& key)
{
    return Lookup(key, [](std::string& val) { /*No-op*/ });
}

bool PhxEngine::CommandLineArgs::GetInteger(std::string const& key, int32_t& value)
{
    return Lookup(key, [&value](std::string& val)
        {
            value = std::stoi(val);
        });
}

bool PhxEngine::CommandLineArgs::GetString(std::string const& key, std::string& value)
{
    return Lookup(key, [&value](std::string& val)
        {
            value = val;
        });
}
