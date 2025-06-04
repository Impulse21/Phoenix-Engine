#pragma once

#include <PhxData/IVirtualFileSystem.h>

namespace phx::data
{

	// Internal structure for a mount point
	struct MountPointInfo 
	{
		enum class Type { Directory, Pak };
		Type type;
		std::string physical_path_normalized; // OS path to the directory or the PAK file
		std::string virtual_prefix_normalized;

		// Add pak support

		MountPointInfo(Type t, std::string phys, std::string virt)
			: type(t)
			, physical_path_normalized(std::move(phys))
			, virtual_prefix_normalized(std::move(virt))
		{
		}
	};

	class VirtualFileSystemImpl final : public IVirtualFileSystem
	{
	public:
		VirtualFileSystemImpl() = default;
		~VirtualFileSystemImpl() override = default;

	public:
		bool Mount(std::string const& virtual_path, std::string const& physical_path) override;
		bool Unmount(std::string const& virtual_path) override;

		Result<AsyncResourceDescriptor> GetResourceDescriptorForAsync(std::string const& virtual_path) const override;
		virtual Result<platform::PlatformFileAttributes> GetPlatformAttributes(std::string const& virtual_path) const override;

		bool Exists(std::string const& virtual_path) override;
		Result<uint64_t> GetUncompressedFileSize(const std::string& virtual_path) const override;
		Result<Blob> ReadFileSynchronous(const std::string& virtual_path) const override;

	private:
		std::string NormalizeVirtualPath(const std::string& path) const;
		std::string NormalizePhysicalPath(const std::string& path) const; // For OS specific normalization

	private:
		std::vector<MountPointInfo> m_mount_points;
	};

	// Helper function (could be in a utility file)
	// This is a simplified path join. Robust path joining is more complex.
	inline std::string JoinPaths(const std::string& p1, const std::string& p2) 
	{
		if (p1.empty())
			return p2;

		if (p2.empty())
			return p1;

		char sep = '/'; // Normalize to one separator type
#ifdef PHX_PLATFORM_WINDOWS
		// sep = '\\'; // Or keep '/' and let Windows handle it
#endif

		std::string result = p1;
		if (result.back() == '/' || result.back() == '\\') 
		{
			if (p2.front() == '/' || p2.front() == '\\') 
			{
				result += p2.substr(1);
			}
			else {
				result += p2;
			}
		}
		else 
		{
			if (p2.front() == '/' || p2.front() == '\\') 
			{
				result += p2;
			}
			else {
				result += sep;
				result += p2;
			}
		}

		// Replace all '\\' with '/' for internal consistency if desired
		std::replace(result.begin(), result.end(), '\\', '/');
		return result;
	}
}

