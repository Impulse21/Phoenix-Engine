#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Core/Log.h>
#include <fstream>

using namespace PhxEngine::Core;

namespace
{
	class Blob : public IBlob
	{
	public:
        Blob(void* Data, size_t size)
            : m_data(Data)
            , m_size(size)
        {}

        ~Blob() override
        {
            if (this->m_data)
            {
                free(this->m_data);
                this->m_data = nullptr;
            }

            this->m_size = 0;
        }

        [[nodiscard]] const void* Data() const override { return this->m_data; }
		[[nodiscard]] size_t Size() const override { return this->m_size; }

	private:
		void* m_data;
		size_t m_size;
	};

    class NativeFileSystem : public IFileSystem
    {
    public:
        bool FileExists(std::filesystem::path const& name) override;
        bool FolderExists(std::filesystem::path const& name) override;
        std::unique_ptr<IBlob> ReadFile(std::filesystem::path const& name) override;
        bool WriteFile(std::filesystem::path const& name, Span<char> Data) override;
    };

    class RelativeFileSystem : public IFileSystem
    {
    public:
        RelativeFileSystem(std::shared_ptr<IFileSystem> fs, const std::filesystem::path& baseBath);

        [[nodiscard]] std::filesystem::path const& GetBasePath() const { return this->m_basePath; }

        bool FileExists(std::filesystem::path const& name) override;
        bool FolderExists(std::filesystem::path const& name) override;
        std::unique_ptr<IBlob> ReadFile(std::filesystem::path const& name) override;
        bool WriteFile(std::filesystem::path const& name, Span<char> Data) override;

    private:
        std::shared_ptr<IFileSystem> m_underlyingFS;
        std::filesystem::path m_basePath;
    };

    class RootFileSystem : public IRootFileSystem
    {
    public:
        void Mount(const std::filesystem::path& path, std::shared_ptr<IFileSystem> fs) override;
        void Mount(const std::filesystem::path& path, const std::filesystem::path& nativePath) override;
        bool Unmount(const std::filesystem::path& path) override;

        bool FileExists(std::filesystem::path const& name) override;
        bool FolderExists(std::filesystem::path const& name) override;
        std::unique_ptr<IBlob> ReadFile(std::filesystem::path const& name) override;
        bool WriteFile(std::filesystem::path const& name, Span<char> Data) override;

    private:
        bool FindMountPoint(const std::filesystem::path& path, std::filesystem::path* pRelativePath, IFileSystem** ppFS);

    private:
        std::vector<std::pair<std::string, std::shared_ptr<IFileSystem>>> m_mountPoints;
    };
}


bool NativeFileSystem::FileExists(std::filesystem::path const& name)
{
	return std::filesystem::exists(name) && std::filesystem::is_regular_file(name);
}

bool NativeFileSystem::FolderExists(std::filesystem::path const& name)
{
	return std::filesystem::exists(name) && std::filesystem::is_directory(name);
}

std::unique_ptr<IBlob> NativeFileSystem::ReadFile(std::filesystem::path const& name)
{
    std::ifstream file(name, std::ios::binary);

    if (!file.is_open())
    {
        // file does not exist or is locked
        return nullptr;
    }

    file.seekg(0, std::ios::end);
    uint64_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
    {
        LOG_CORE_ERROR("File larger then size_t");
        return nullptr;
    }

    char* Data = static_cast<char*>(malloc(size));

    if (Data == nullptr)
    {
        LOG_CORE_ERROR("Out of memory");
        return nullptr;
    }

    file.read(Data, size);

    if (!file.good())
    {
        LOG_CORE_ERROR("Reading error");
        free(Data);
        return nullptr;
    }

    return std::make_unique<Blob>(Data, size);
}

bool NativeFileSystem::WriteFile(std::filesystem::path const& name, Span<char> Data)
{
    std::ofstream file(name, std::ios::binary);

    if (!file.is_open())
    {
        LOG_CORE_ERROR("File does not exist or is locked");
        return false;
    }

    if (Data.Size() > 0)
    {
        file.write(Data.begin(), static_cast<std::streamsize>(Data.Size()));
    }

    if (!file.good())
    {
        LOG_CORE_ERROR("Failed to write file.");
        return false;
    }

    return true;
}

RelativeFileSystem::RelativeFileSystem(std::shared_ptr<IFileSystem> fs, const std::filesystem::path& baseBath)
    : m_underlyingFS(std::move(fs))
    , m_basePath(baseBath.lexically_normal())
{
}

bool RelativeFileSystem::FileExists(std::filesystem::path const& name)
{
    return this->m_underlyingFS->FileExists(this->m_basePath / name.relative_path());
}

bool RelativeFileSystem::FolderExists(std::filesystem::path const& name)
{
    return this->m_underlyingFS->FolderExists(this->m_basePath / name.relative_path());
}

std::unique_ptr<IBlob> RelativeFileSystem::ReadFile(std::filesystem::path const& name)
{
    return this->m_underlyingFS->ReadFile(this->m_basePath / name.relative_path());
}

bool RelativeFileSystem::WriteFile(std::filesystem::path const& name, Span<char> Data)
{
    return this->m_underlyingFS->WriteFile(this->m_basePath / name.relative_path(), Data);
}

void RootFileSystem::Mount(const std::filesystem::path& path, std::shared_ptr<IFileSystem> fs)
{
    if (this->FindMountPoint(path, nullptr, nullptr))
    {
        LOG_CORE_ERROR("Cannot mount a filesystem at {0}: there is another FS that includes this path", path.generic_string().c_str());
        return;
    }

    this->m_mountPoints.push_back(std::make_pair(path.lexically_normal().generic_string(), fs));
}

void RootFileSystem::Mount(const std::filesystem::path& path, const std::filesystem::path& nativePath)
{
    this->Mount(path, std::make_shared<RelativeFileSystem>(std::make_shared<NativeFileSystem>(), nativePath));
}

bool RootFileSystem::Unmount(const std::filesystem::path& path)
{
    std::string spath = path.lexically_normal().generic_string();

    for (size_t index = 0; index < this->m_mountPoints.size(); index++)
    {
        if (this->m_mountPoints[index].first == spath)
        {
            this->m_mountPoints.erase(this->m_mountPoints.begin() + index);
            return true;
        }
    }

    return false;
}

bool RootFileSystem::FileExists(std::filesystem::path const& name)
{
    std::filesystem::path relativePath;
    IFileSystem* fs = nullptr;

    if (this->FindMountPoint(name, &relativePath, &fs))
    {
        return fs->FileExists(relativePath);
    }

    return false;
}

bool RootFileSystem::FolderExists(std::filesystem::path const& name)
{
    std::filesystem::path relativePath;
    IFileSystem* fs = nullptr;

    if (this->FindMountPoint(name, &relativePath, &fs))
    {
        return fs->FolderExists(relativePath);
    }

    return false;
}

std::unique_ptr<IBlob> RootFileSystem::ReadFile(std::filesystem::path const& name)
{
    std::filesystem::path relativePath;
    IFileSystem* fs = nullptr;

    if (this->FindMountPoint(name, &relativePath, &fs))
    {
        return fs->ReadFile(relativePath);
    }

    return nullptr;
}

bool RootFileSystem::WriteFile(std::filesystem::path const& name, Span<char> Data)
{
    std::filesystem::path relativePath;
    IFileSystem* fs = nullptr;

    if (this->FindMountPoint(name, &relativePath, &fs))
    {
        return fs->WriteFile(relativePath, Data);
    }

    return false;
}

bool RootFileSystem::FindMountPoint(const std::filesystem::path& path, std::filesystem::path* pRelativePath, IFileSystem** ppFS)
{
    std::string spath = path.lexically_normal().generic_string();

    for (auto it : this->m_mountPoints)
    {
        if (spath.find(it.first, 0) == 0 && ((spath.length() == it.first.length()) || (spath[it.first.length()] == '/')))
        {
            if (pRelativePath)
            {
                std::string relative = spath.substr(it.first.size() + 1);
                *pRelativePath = relative;
            }

            if (ppFS)
            {
                *ppFS = it.second.get();
            }

            return true;
        }
    }

    return false;
}

std::unique_ptr<IFileSystem> PhxEngine::Core::CreateNativeFileSystem()
{
    return std::make_unique<NativeFileSystem>();
}

std::unique_ptr<IFileSystem> PhxEngine::Core::CreateRelativeFileSystem(std::shared_ptr<IFileSystem> fs, const std::filesystem::path& basePath)
{
    return std::make_unique<RelativeFileSystem>(fs, basePath);
}

std::unique_ptr<IRootFileSystem> PhxEngine::Core::CreateRootFileSystem()
{
    return std::make_unique<RootFileSystem>();
}

std::unique_ptr<IBlob> PhxEngine::Core::CreateBlob(void* Data, size_t size)
{
    return std::make_unique<Blob>(Data, size);
}
