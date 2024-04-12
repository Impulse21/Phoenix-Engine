#include <PhxEngine/Core/ProjectSettings.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <streambuf>

#include <yaml-cpp/yaml.h>

constexpr uint32_t CurrentPrjectSettingVersion = 1;

PhxEngine::ProjectSettings::ProjectSettings()
{
	if (sSingleton)
	{
		throw std::runtime_error("Project settings file is a singleton and it's already been iniitalized");
	}

	sSingleton = this;
}

PhxEngine::ProjectSettings::~ProjectSettings()
{
	sSingleton = nullptr;
}

bool PhxEngine::ProjectSettings::Startup(std::string const& path)
{
	bool retVal = this->LoadSettings(path);
	return retVal;
}

void PhxEngine::ProjectSettings::Shutdown()
{
}

bool PhxEngine::ProjectSettings::LoadSettings(std::string const& path)
{
	// Load  Settings file

	// TODO: File Accessing
	RefCountPtr<FileAccess> settingsFile = FileAccess::Open(path, AccessFlags::Read);

	if (settingsFile == nullptr)
	{
		return false;
	}

	this->m_resourcePath = FileAccess::GetDirectory(path);

	std::unique_ptr<IBlob> data = settingsFile->ReadFile();

	BlobStream buf(data.get());
	std::istream istr(&buf);
	YAML::Node n = YAML::Load(istr);
	uint32_t version = n["version"].as<uint32_t>();

	if (version != CurrentPrjectSettingVersion)
	{
		return false;
	}

	this->m_name = n["name"].as<std::string>();
}
