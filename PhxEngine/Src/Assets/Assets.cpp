#include <PhxEngine/Assets/Assets.h>

#include <PhxEngine/Core/Threading.h>
#include <PhxEngine/Core/Log.h>
#include <fstream>

using namespace PhxEngine::Core;
using namespace PhxEngine::Assets;

Asset::Asset()
    : m_memoryUsage(0)
    , m_asyncLoadState(AsyncLoadState::Done)
{
}

void Asset::SetName(std::string const & name)
{
    this->m_name= name;
    this->m_nameHash = Core::StringHash(name);
}

void Asset::SetMemoryUsage(int32_t size)
{
    assert(size >= 0);
    this->m_memoryUsage = size;
}

void Asset::ResetUseTimer()
{
   this->m_useTimer.Begin();
}

void Asset::SetAsyncLoadState(AsyncLoadState newState)
{
    this->m_asyncLoadState = newState;
}

PhxEngine::Core::TimeStep Asset::GetUseTimer()
{
#if false
    // If more references than the resource cache, return always 0 & reset the timer
    if (this->Refs() > 1)
    {
        this->ResetUseTimer();
        return 0;
    }
    else
#endif
        return this->m_useTimer.Elapsed();
}

bool PhxEngine::Assets::Asset::Load(std::ifstream& fileStream)
{
    // TODO: Add Profiler
    this->SetAsyncLoadState(Threading::IsMainThread() ? AsyncLoadState::Done : AsyncLoadState::Loading);

    bool success = this->BeginLoad();
    if (success)
        success &= this->EndLoad();

    this->SetAsyncLoadState(AsyncLoadState::Done);

    return success;
}
bool PhxEngine::Assets::Asset::Save(std::ofstream& fileStream) const
{
    PHX_LOG_CORE_ERROR("Save not supported for %s", this->GetTypeName().c_str());
    return false;
}


bool PhxEngine::Assets::Asset::LoadFile(std::string const& fileName)
{
    std::ifstream fileStream(fileName, std::ios::binary);

    if (!fileStream.is_open())
    {
        PHX_LOG_CORE_ERROR("Unable to open file %s for %s", fileName.c_str(), this->GetTypeName().c_str());
        return false;
    }

    return this->Load(fileStream);
}

bool PhxEngine::Assets::Asset::SaveFile(std::string const& filename) const
{
    std::ofstream fileStream(filename, std::ios::binary);

    if (!fileStream.is_open())
    {
        PHX_LOG_CORE_ERROR("Unable to open file %s for %s", filename.c_str(), this->GetTypeName().c_str());
        return false;
    }

    return this->Save(fileStream);
}
