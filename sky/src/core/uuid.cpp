#include "uuid.h"

namespace sky
{
static std::random_device s_randomDevice;
static std::mt19937_64 s_randomEngine(s_randomDevice());
static std::uniform_int_distribution<uint64_t> s_uniformDistribution;

UUID::UUID() : m_uuid(NULL_UUID) {}

UUID::UUID(uint64_t uuid) : m_uuid(uuid) {}

const UUID UUID::generate() 
{
	return s_uniformDistribution(s_randomEngine);
}

const std::string UUID::toString() const { return std::to_string(m_uuid); }
}