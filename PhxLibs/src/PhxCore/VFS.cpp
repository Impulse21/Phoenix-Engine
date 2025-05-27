#include "PhxCore_pch.h"
#include "PhxCore/VFS.h"

#include <fstream>

#ifndef PHX_PLATFORM_WINDOWS
#include <unistd.h>
#include <cstdio>
#include <climits>
#else
#define PATH_MAX MAX_PATH
#endif // _WIN32

namespace phx
{
    bool NativeFileSystem::FileExists(std::filesystem::path const& name)
    {
        return std::filesystem::exists(name) && std::filesystem::is_regular_file(name);
    }

    bool NativeFileSystem::FolderExists(std::filesystem::path const& name)
    {
        return std::filesystem::exists(name) && std::filesystem::is_directory(name);
    }

    bool NativeFileSystem::FolderCreate(std::filesystem::path const& name)
    {
        std::filesystem::path parentDir = name.parent_path();
        if (FolderExists(parentDir))
            return true;

        return std::filesystem::create_directory(parentDir);
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
        uint64_t size = static_cast<uint64_t>(file.tellg());
        file.seekg(0, std::ios::beg);

        if (size > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
        {
            PHX_CORE_ERROR("File larger then size_t");
            return nullptr;
        }

        char* Data = static_cast<char*>(malloc(size));

        if (Data == nullptr)
        {
            PHX_CORE_ERROR("Out of memory");
            return nullptr;
        }

        file.read(Data, size);

        if (!file.good())
        {
            PHX_CORE_ERROR("Reading error");
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
            PHX_CORE_ERROR("File does not exist or is locked");
            return false;
        }

        if (Data.Size() > 0)
        {
            file.write(Data.begin(), static_cast<std::streamsize>(Data.Size()));
        }

        if (!file.good())
        {
            PHX_CORE_ERROR("Failed to write file.");
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

    bool RelativeFileSystem::FolderCreate(std::filesystem::path const& name)
    {
        return this->m_underlyingFS->FolderCreate(this->m_basePath / name.relative_path());
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
            PHX_CORE_ERROR("Cannot mount a filesystem at %s: there is another FS that includes this path", path.generic_string().c_str());

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

    bool RootFileSystem::FolderCreate(std::filesystem::path const& name)
    {
        std::filesystem::path relativePath;
        IFileSystem* fs = nullptr;

        if (this->FindMountPoint(name, &relativePath, &fs))
        {
            return fs->FolderCreate(relativePath);
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

    std::filesystem::path RootFileSystem::ResolvePath(std::filesystem::path const& name)
    {
        std::filesystem::path relativePath;
        IFileSystem* fs = nullptr;

        if (this->FindMountPoint(name, &relativePath, &fs))
        {
            return fs->ResolvePath(relativePath);
        }

        return name;
    }

    bool RootFileSystem::FindMountPoint(const std::filesystem::path& path, std::filesystem::path* pRelativePath, IFileSystem** ppFS)
    {
        std::string spath = path.lexically_normal().generic_string();

        for (const auto& [prefix, fs] : m_mountPoints)
        {
            if (spath.find(prefix, 0) == 0 && ((spath.length() == prefix.length()) || (spath[prefix.length() - 1] == '/')))
            {
                if (pRelativePath)
                {
                    std::string relative = spath.substr(prefix.size());
                    *pRelativePath = relative;
                }

                if (ppFS)
                {
                    *ppFS = fs.get();
                }

                return true;
            }
        }

        return false;
    }
}

namespace phx::FileSystemFactory
{
    std::unique_ptr<IFileSystem> CreateNativeFileSystem()
    {
        return std::make_unique<NativeFileSystem>();
    }

    std::unique_ptr<IFileSystem> CreateRelativeFileSystem(std::shared_ptr<IFileSystem> fs, const std::filesystem::path& basePath)
    {
        return std::make_unique<RelativeFileSystem>(fs, basePath);
    }

    std::unique_ptr<IRootFileSystem> CreateRootFileSystem()
    {
        return std::make_unique<RootFileSystem>();
    }

    std::unique_ptr<IBlob> CreateBlob(void* Data, size_t size)
    {
        return std::make_unique<Blob>(Data, size);
    }
}

namespace phx::FileSystem
{
    std::filesystem::path GetWorkingDirectory()
    {
        return std::filesystem::current_path();
    }

    std::filesystem::path GetDirectoryWithExecutable()
    {
        char path[PATH_MAX] = { 0 };
#ifdef PHX_PLATFORM_WINDOWS
        if (GetModuleFileNameA(nullptr, path, PATH_MAX) == 0)
            return "";
#else // _WIN32
        // /proc/self/exe is mostly linux-only, but can't hurt to try it elsewhere
        if (readlink("/proc/self/exe", path, std::size(path)) <= 0)
        {
            // portable but assumes executable dir == cwd
            if (!getcwd(path, std::size(path)))
                return ""; // failure
        }
#endif // PHX_PLATFORM_WINDOWS

        std::filesystem::path result = path;
        result = result.parent_path();

        return result;
    }

    std::string GetFileNameWithoutExt(std::string const& path)
    {
        return std::filesystem::path(path).stem().generic_string();
    }

    std::string GetFileExt(std::string const& path)
    {
        return std::filesystem::path(path).extension().generic_string();
    }
}

namespace phx::FileSystem
{
    std::string GetDirectory(const char* path)
    {
        if (!path)
            return nullptr;

        const char* lastSlash = nullptr;
        for (const char* p = path; *p != '\0'; ++p)
        {
            if (*p == '/' || *p == '\\')
            {
                lastSlash = p;
            }
        }

        if (lastSlash)
            return std::string(path, lastSlash + 1); // include the slash
        else
            return {};
    }
}