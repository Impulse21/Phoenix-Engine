#include "pch.h"
#include "phxUUID.h"

#include <random>

#include <unordered_map>

using namespace phx;

namespace
{
	static std::random_device sRandomDevice;
	static std::mt19937_64 sEngine(sRandomDevice());
	static std::uniform_int_distribution<uint64_t> sUniformDistribution;
}

phx::UUID::UUID()
	: m_uuid(sUniformDistribution(sEngine))
{
}

