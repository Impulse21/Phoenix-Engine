#include "CVar.h"

using namespace phx;


void CVar::Initialize(Span<char*> args)
{
    LoadConfig(phx.cfg);
    for (size_t i = 0; i < args.size(); ++i)
    {
        const char* arg = args[i];
        // TODO: PARSE
    }
}

void CVar::Shutdown()
{

}

void CVar::LoadConfig(const char* /*file*/)
{

}