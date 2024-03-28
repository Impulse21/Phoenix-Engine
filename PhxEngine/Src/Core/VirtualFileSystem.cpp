#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Core/Logger.h>
#include <fstream>

#ifdef WIN32
#include <Shlwapi.h>
#else
extern "C" {
#include <glob.h>
}
#endif // _WIN32

using namespace PhxEngine;

namespace
{

    static int EnumerateNativeFiles(const char* pattern, bool directories, EnumCallback callback)
    {
#ifdef WIN32

        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(pattern, &findData);

        if (hFind == INVALID_HANDLE_VALUE)
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
                return 0;

            return FileStatus::Failed;
        }

        int numEntries = 0;

        do
        {
            bool isDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            bool isDot = strcmp(findData.cFileName, ".") == 0;
            bool isDotDot = strcmp(findData.cFileName, "..") == 0;

            if ((isDirectory == directories) && !isDot && !isDotDot)
            {
                callback(findData.cFileName);
                ++numEntries;
            }
        } while (FindNextFileA(hFind, &findData) != 0);

        FindClose(hFind);

        return numEntries;

#else // WIN32

        glob64_t glob_matches;
        int globResult = glob64(pattern, 0 /*flags*/, nullptr /*errfunc*/, &glob_matches);

        if (globResult == 0)
        {
            int numEntries = 0;

            for (int i = 0; i < glob_matches.gl_pathc; ++i)
            {
                const char* globentry = (glob_matches.gl_pathv)[i];
                std::error_code ec, ec2;
                std::filesystem::directory_entry entry(globentry, ec);
                if (!ec)
                {
                    if (directories == entry.is_directory(ec2) && !ec2)
                    {
                        callback(entry.path().filename().native());
                        ++numEntries;
                    }
                }
            }
            globfree64(&glob_matches);

            return numEntries;
        }

        if (globResult == GLOB_NOMATCH)
            return 0;

        return status::Failed;

#endif // WIN32
    }


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
        PHX_LOG_CORE_ERROR("File larger then size_t");
        return nullptr;
    }

    char* Data = static_cast<char*>(malloc(size));

    if (Data == nullptr)
    {
        PHX_LOG_CORE_ERROR("Out of memory");
        return nullptr;
    }

    file.read(Data, size);

    if (!file.good())
    {
        PHX_LOG_CORE_ERROR("Reading error");
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
        PHX_LOG_CORE_ERROR("File does not exist or is locked");
        return false;
    }

    if (Data.Size() > 0)
    {
        file.write(Data.begin(), static_cast<std::streamsize>(Data.Size()));
    }

    if (!file.good())
    {
        PHX_LOG_CORE_ERROR("Failed to write file.");
        return false;
    }

    return true;
}

int NativeFileSystem::EnumerateFiles(const std::filesystem::path& path, const std::vector<std::string>& extensions, EnumCallback callback, bool allowDuplicates)
{
    (void)allowDuplicates;

    if (extensions.empty())
    {
        std::string pattern = (path / "*").generic_string();
        return EnumerateNativeFiles(pattern.c_str(), false, callback);
    }

    int numEntries = 0;
    for (const auto& ext : extensions)
    {
        std::string pattern = (path / ("*" + ext)).generic_string();
        int result = EnumerateNativeFiles(pattern.c_str(), false, callback);

        if (result < 0)
            return result;

        numEntries += result;
    }

    return numEntries;
}

int NativeFileSystem::EnumerateDirectories(const std::filesystem::path& path, EnumCallback callback, bool allowDuplicates)
{
    (void)allowDuplicates;

    std::string pattern = (path / "*").generic_string();
    return EnumerateNativeFiles(pattern.c_str(), true, callback);
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

int RelativeFileSystem::EnumerateFiles(const std::filesystem::path& path, const std::vector<std::string>& extensions, EnumCallback callback, bool allowDuplicates)
{
    return this->m_underlyingFS->EnumerateFiles(this->m_basePath / path.relative_path(), extensions, callback, allowDuplicates);
}

int RelativeFileSystem::EnumerateDirectories(const std::filesystem::path& path, EnumCallback callback, bool allowDuplicates)
{
    return this->m_underlyingFS->EnumerateDirectories(this->m_basePath / path.relative_path(), callback, allowDuplicates);
}

void RootFileSystem::Mount(const std::filesystem::path& path, std::shared_ptr<IFileSystem> fs)
{
    if (this->FindMountPoint(path, nullptr, nullptr))
    {
        PHX_LOG_CORE_ERROR("Cannot mount a filesystem at %s: there is another FS that includes this path", path.generic_string().c_str());
        
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

int RootFileSystem::EnumerateFiles(const std::filesystem::path& path, const std::vector<std::string>& extensions, EnumCallback callback, bool allowDuplicates)
{
    std::filesystem::path relativePath;
    IFileSystem* fs = nullptr;

    if (this->FindMountPoint(path, &relativePath, &fs))
    {
        return fs->EnumerateFiles(relativePath, extensions, callback, allowDuplicates);
    }

    return FileStatus::PathNotFound;
}

int RootFileSystem::EnumerateDirectories(const std::filesystem::path& path, EnumCallback callback, bool allowDuplicates)
{
    std::filesystem::path relativePath;
    IFileSystem* fs = nullptr;

    if (this->FindMountPoint(path, &relativePath, &fs))
    {
        return fs->EnumerateDirectories(relativePath, callback, allowDuplicates);
    }

    return FileStatus::PathNotFound;
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

std::unique_ptr<IFileSystem> PhxEngine::FileSystemFactory::CreateNativeFileSystem()
{
    return std::make_unique<NativeFileSystem>();
}

std::unique_ptr<IFileSystem> PhxEngine::FileSystemFactory::CreateRelativeFileSystem(std::shared_ptr<IFileSystem> fs, const std::filesystem::path& basePath)
{
    return std::make_unique<RelativeFileSystem>(fs, basePath);
}

std::unique_ptr<IRootFileSystem> PhxEngine::FileSystemFactory::CreateRootFileSystem()
{
    return std::make_unique<RootFileSystem>();
}

std::unique_ptr<IBlob> PhxEngine::FileSystemFactory::CreateBlob(void* Data, size_t size)
{
    return std::make_unique<Blob>(Data, size);
}

std::string PhxEngine::FileSystem::GetFileNameWithoutExt(std::string const& path)
{
    return std::filesystem::path(path).stem().generic_string();
}

std::string PhxEngine::FileSystem::GetFileExt(std::string const& path)
{
    return std::filesystem::path(path).extension().generic_string();
}

std::string PhxEngine::FileSystem::GetFileExt(std::string_view path)
{
    return std::filesystem::path(path).extension().generic_string();
}
