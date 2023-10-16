#include <PhxEngine/Assets/AssetStore.h>

#include <PhxEngine/Core/StringHash.h>
#include <unordered_map>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Assets;

namespace
{
	std::unordered_map<uint32_t, std::weak_ptr<IMesh>> m_meshStore;
	std::unordered_map<uint32_t, std::weak_ptr<IMesh>> m_textureStore;
	std::unordered_map<uint32_t, std::weak_ptr<IMesh>> m_materialStore;
}

void AssetStore::Initialize()
{
}

void AssetStore::Finalize()
{
	m_meshStore.clear();
	m_materialStore.clear();
	m_textureStore.clear();
}

