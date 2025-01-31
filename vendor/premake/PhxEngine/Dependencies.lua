
-- phx Dependencies

-- TODO Adjust this to be better self contained

-- Example
--AgilityLibrary =
--{
--	includeDirs = LibAgility..IncludeDirectory..'include',
--	libDirs     = LibAgility..BinaryDirectory,
--	dlls        =
--	{
--		LibAgility..BinaryDirectory..'x64/D3D12Core.dll',
--		LibAgility..BinaryDirectory..'x64/d3d12SDKLayers.dll'
--	}
--}

IncludeDir = {}
IncludeDir["ImGui"]         = "%{wks.location}/../../phoenix/vendor/ImGui"
IncludeDir["D3D12MA"]       = "%{wks.location}/../../phoenix/vendor/D3D12MA"
IncludeDir["ENTT"]          = "%{wks.location}/../../phoenix/vendor/entt"
IncludeDir["AgilitySDK"]    = "%{wks.location}/../PrebuiltLibs/agility_1.614.1/include"
IncludeDir["DXC"]           = "%{wks.location}/../PrebuiltLibs/dxc_2024_07_31_clang_cl/inc"
IncludeDir["DStorage"]      = "%{wks.location}/../PrebuiltLibs/directstorage_1.2.2/include"

LibraryDir = {}
LibraryDir["AgilitySDK_Win64"]      = "%{wks.location}../PrebuiltLibs/agility_1.614.1/bin/x64"
LibraryDir["DStorage_Win64"]        = "%{wks.location}../PrebuiltLibs/directstorage_1.2.2/bin/x64"
LibraryDir["DStorage_lib_Win64"]    = "%{wks.location}/../PrebuiltLibs/directstorage_1.2.2/lib/x64"

DynamicLibrary = {}
DynamicLibrary["D3D12Core"]        = LibraryDir["AgilitySDK_Win64"].."/D3D12Core.dll"
DynamicLibrary["d3d12SDKLayers"]   = LibraryDir["AgilitySDK_Win64"].."/d3d12SDKLayers.dll"
DynamicLibrary["DStorage"]         = LibraryDir["DStorage_Win64"].."/dstorage.dll"
DynamicLibrary["DStorageCore"]     = LibraryDir["DStorage_Win64"].."/dstoragecore.dll"

Library = {}
Library["D3D12"]    = "d3d12.lib"
Library["DXGI"]     = "dxgi.lib"
Library["DXGUID"]   = "dxguid.lib"

Library["DStorage"]   = "dstorage.lib"

-- Windows
Library["WinSock"] = "Ws2_32.lib"
Library["WinMM"] = "Winmm.lib"
Library["WinVersion"] = "Version.lib"
Library["BCrypt"] = "Bcrypt.lib"
