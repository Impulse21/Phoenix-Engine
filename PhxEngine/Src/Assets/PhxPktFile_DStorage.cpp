#include "PhxPktFile_DStorage.h"

PhxEngine::Assets::PhxPktFile_DStorage::PhxPktFile_DStorage(std::string const& filename)
	: PhxPktFile(filename)
	// , m_headerLoaded(EventWait::Create<PhxPktFile_DStorage, >())
{
}

void PhxEngine::Assets::PhxPktFile_DStorage::StartMetadataLoad()
{
}
