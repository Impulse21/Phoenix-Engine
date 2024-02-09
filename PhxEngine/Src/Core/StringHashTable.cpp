#include <PhxEngine/Core/StringHashTable.h>
#include <fstream>
#include <json.hpp>

#include <PhxEngine/Core/Log.h>

using namespace PhxEngine::Core;
using namespace nlohmann;

namespace
{
	std::unordered_map<Hash32, std::string> m_stringLUT;
}

void PhxEngine::Core::StringHashTable::Import(std::string const& file)
{
	std::ifstream f(file);
	if (!f.is_open())
	{
		PHX_LOG_CORE_ERROR("Failed to open string hash table file");
		return;
	}

	json j = json::parse(f);
	for (auto el : j["hash_table"])
	{
		m_stringLUT.emplace(el["hash"].get<Hash32>(), el["string_value"].get<std::string>());
	}
}

void PhxEngine::Core::StringHashTable::Export(std::string const& file)
{
	std::vector<json> entries;
	entries.reserve(m_stringLUT.size());

	for (auto& [key, value] : m_stringLUT)
	{
		auto& entry = entries.emplace_back();
		entry["hash"] = key;
		entry["string_value"] = value;
	}

	json j;
	j["hash_table"] = entries;

	std::ofstream f(file);
	f << std::setw(4) << j << std::endl;
}

std::string const& PhxEngine::Core::StringHashTable::GetName(StringHash hash)
{
	auto itr = m_stringLUT.find(hash);
	if (itr == m_stringLUT.end())
		return "";

	return itr->second;
}

void PhxEngine::Core::StringHashTable::RegisterHash(StringHash hash, std::string const& name)
{
	m_stringLUT[hash] = name;
}
