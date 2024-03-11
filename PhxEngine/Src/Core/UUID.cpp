#include <PhxEngine/Core/UUID.h>

#include <random>

#include <unordered_map>

using namespace PhxEngine;

namespace
{
	static std::random_device sRandomDevice;
	static std::mt19937_64 sEngine(sRandomDevice());
	static std::uniform_int_distribution<uint64_t> sUniformDistribution;
}

UUID::UUID()
	: m_uuid(sUniformDistribution(sEngine))
{
}

UUID::UUID(uint64_t uuid)
	: m_uuid(uuid)
{
}