#include <PhxEngine/Core/FileSystem.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Asserts.h>

#include <fstream>

using namespace PhxEngine::Core;

PhxEngine::Core::Blob::Blob(void* data, size_t size)
	: m_data(data)
	, m_size(size)
{
}

PhxEngine::Core::Blob::~Blob()
{
	if (this->m_data)
	{
		free(this->m_data);
		this->m_data = nullptr;
	}

	this->m_size = 0;
}

bool PhxEngine::Core::NativeFileSystem::FileExists(std::filesystem::path const& filename)
{
	return std::filesystem::exists(filename) && std::filesystem::is_regular_file(filename);
}

std::shared_ptr<IBlob> PhxEngine::Core::NativeFileSystem::ReadFile(std::filesystem::path const& filename)
{
	std::ifstream file(filename, std::ios::binary);

	if (!file.is_open())
	{
		// TODO: Add filename
		LOG_CORE_ERROR("Failed to load file");
		return nullptr;
	}

	file.seekg(0, std::ios::end);
	uint64_t size = file.tellg();
	file.seekg(0, std::ios::beg);

	if (size > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
	{
		// file larger than size_t
		PHX_ASSERT(false);
		return nullptr;
	}

	char* data = static_cast<char*>(malloc(size));

	if (data == nullptr)
	{
		// out of memory
		PHX_ASSERT(false);
		return nullptr;
	}

	file.read(data, size);

	if (!file.good())
	{
		// reading error
		PHX_ASSERT(false);
		return nullptr;
	}

	return std::make_shared<Blob>(data, size);
}
