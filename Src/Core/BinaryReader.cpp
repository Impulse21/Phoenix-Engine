#include <PhxEngine/Core/BinaryReader.h>

#include<PhxEngine/Core/FileSystem.h>

using namespace PhxEngine::Core;

PhxEngine::Core::BinaryReader::BinaryReader(std::unique_ptr<IBlob> binaryBlob) noexcept
	: m_data(std::move(binaryBlob))
{
	this->m_currentPos = reinterpret_cast<uint8_t const*>(this->m_data->Data());
	this->m_endPos = reinterpret_cast<uint8_t const*>(this->m_data->Data()) + this->m_data->Size();
}
