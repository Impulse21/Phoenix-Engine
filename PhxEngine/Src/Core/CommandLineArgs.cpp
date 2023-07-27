#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Core/String.h>

PhxEngine::Core::CommandLineArgs::CommandLineArgs(int argc, const char** argv)
{
    GatherArgs(size_t(argc), [argv](size_t i) -> const char*
        {
#if 0
            std::string str;
            Helpers::StringConvert(argv[i], str);
            return str;
#endif
            return argv[i];
        });
}

bool PhxEngine::Core::CommandLineArgs::HasArg(std::string const& key)
{
    return this->Lookup(key, [](std::string& val) { /*No-op*/ });
}

bool PhxEngine::Core::CommandLineArgs::GetInteger(std::string const& key, int32_t& value)
{
    return this->Lookup(key, [&value](std::string& val)
        {
            value = std::stoi(val);
        });
}

bool PhxEngine::Core::CommandLineArgs::GetString(std::string const& key, std::string& value)
{
    return this->Lookup(key, [&value](std::string& val)
        {
            value = val;
        });
}
