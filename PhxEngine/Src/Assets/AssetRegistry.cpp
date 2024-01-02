#include <PhxEngine/Assets/AssetRegistry.h>

#include <PhxEngine/Core/Threading.h>

using namespace PhxEngine;
using namespace PhxEngine::Assets;


AssetsRegistry::~AssetsRegistry()
{
}

void AssetsRegistry::ReleaseAssets(std::string const& partialName, bool force)
{
}

void AssetsRegistry::ReleaseAllAssets(bool force)
{
}

std::shared_ptr<Asset> AssetsRegistry::GetTempAsset(Core::StringHash type, std::string const& name, bool sendEventOnFailure)
{
	return std::shared_ptr<Asset>();
}

bool AssetsRegistry::BackgroundLoadAsset(Core::StringHash type, std::string const& name, bool sendEventOnFailure, Asset* caller)
{
	return false;
}


bool PhxEngine::Assets::AssetsRegistry::AddAssetDir(std::string const& pathName)
{
    std::scoped_lock lock(this->m_registryMutex);

    auto* fileSystem = this->m_fileSystem.get();
    if (!fileSystem || !fileSystem->FolderExists(pathName))
    {
        PHX_LOG_CORE_ERROR("Could not open directory %s", pathName);
        return false;
    }

    // Check that the same path does not already exist
    for (const std::string& resourceDir : this->m_AssetDirectories)
    {
        if (resourceDir == pathName)
            return true;
    }

    this->m_AssetDirectories.push_back(pathName);
    PHX_LOG_CORE_INFO("Added resource path %s", pathName.c_str());
    return true;
}

bool PhxEngine::Assets::AssetsRegistry::AddManualAsset(Asset* asset)
{
    if (!asset)
    {
        PHX_LOG_CORE_ERROR("Null manual resource");
        return false;
    }

    const std::string& name = asset->GetName();
    if (name.empty())
    {
        PHX_LOG_CORE_ERROR("Manual resource with empty name, can not add");
        return false;
    }

    asset->ResetUseTimer();
    this->m_assetGroups[asset->GetType()].Assets[asset->GetNameHash()] = std::shared_ptr<Asset>(asset);
    this->UpdateResourceGroup(asset->GetType());
    return true;
}

void PhxEngine::Assets::AssetsRegistry::RemoveAssetDir(std::string const& pathName)
{
    std::scoped_lock lock(this->m_registryMutex);
    for (auto itr = this->m_AssetDirectories.begin(); itr != this->m_AssetDirectories.end(); itr++)
    {
        if (*itr == pathName)
        {
            this->m_AssetDirectories.erase(itr);
            PHX_LOG_CORE_ERROR("Removed Asset Path %s", pathName);
            return;
        }
    }
}

void PhxEngine::Assets::AssetsRegistry::ReleaseAsset(Core::StringHash type, std::string const& name, bool force)
{
    Core::StringHash nameHash(name);
    const std::shared_ptr<Asset>& existingRes = this->FindAsset(type, nameHash);
    if (!existingRes)
        return;

    this->m_assetGroups[type].Assets.erase(nameHash);
    UpdateResourceGroup(type);
}

size_t AssetsRegistry::GetNumBackgroundLoadAssets() const
{
	return size_t();
}

void AssetsRegistry::GetAssets(Core::Span<Asset*> result, Core::StringHash type) const
{
}

const std::shared_ptr<Asset>& AssetsRegistry::FindAsset(Core::StringHash nameHash)
{
    std::scoped_lock lock(this->m_registryMutex);

    for (auto groupItr = this->m_assetGroups.begin(); groupItr != this->m_assetGroups.end(); ++groupItr)
    {
        auto assetItr  = groupItr->second.Assets.find(nameHash);
        if (assetItr != groupItr->second.Assets.end())
            return assetItr->second;
    }

    return nullptr;
}

void AssetsRegistry::UpdateResourceGroup(Core::StringHash type)
{
    auto groupItr = this->m_assetGroups.find(type);
    if (groupItr == this->m_assetGroups.end())
        return;

    for (;;)
    {
        unsigned totalSize = 0;
        unsigned oldestTimer = 0;
        auto oldestResourceItr = groupItr->second.Assets.end();

        for (auto assetItr = groupItr->second.Assets.begin();
            assetItr != groupItr->second.Assets.end(); ++assetItr)
        {
            totalSize += assetItr->second->GetMemoryUse();
            unsigned useTimer = assetItr->second->GetUseTimer();
            if (useTimer > oldestTimer)
            {
                oldestTimer = useTimer;
                oldestResourceItr = assetItr;
            }
        }

        groupItr->second.MemoryUsage = totalSize;

        // If memory budget defined and is exceeded, remove the oldest resource and loop again
        // (resources in use always return a zero timer and can not be removed)
        if (groupItr->second.MemoryBudget && groupItr->second.MemoryUsage> groupItr->second.MemoryBudget &&
            oldestResourceItr != groupItr->second.Assets.end())
        {
            PHX_LOG_CORE_INFO("Resource group %s over memory budget, releasing resource %s", 
                oldestResourceItr->second->GetTypeName(),
                oldestResourceItr->second->GetName());
            groupItr->second.Assets.erase(oldestResourceItr);
        }
        else
            break;
    }
}

const std::shared_ptr<Asset>& PhxEngine::Assets::AssetsRegistry::FindAsset(Core::StringHash type, Core::StringHash nameHash)
{
    std::scoped_lock lock(this->m_registryMutex);

    auto groupItr = this->m_assetGroups.find(type);
    if (groupItr == this->m_assetGroups.end())
        return nullptr;

    auto assetItr = groupItr->second.Assets.find(nameHash);
    if (assetItr == groupItr->second.Assets.end())
        return nullptr;

    return assetItr->second;
}
