
-- phx Dependencies

IncludeDir = {}
IncludeDir["ImGui"]         = "%{wks.location}/../../phoenix/vendor/ImGui"
IncludeDir["D3D12MA"]       = "%{wks.location}/../..//phoenix/vendor/D3D12MA"
IncludeDir["AgilitySDK"]    = "%{wks.location}/../PrebuiltLibs/agility_1.614.1/include"
IncludeDir["DXC"]           = "%{wks.location}/../PrebuiltLibs/dxc_2024_07_31_clang_cl/inc"

LibraryDir = {}
LibraryDir["AgilitySDK_Win64"]    = "%{wks.location}/../PrebuiltLibs/agility_1.614.1/bin/x64"

Library = {}
Library["D3D12"]    = "d3d12.lib"
Library["DXGI"]     = "dxgi.lib"
Library["DXGUID"]   = "dxguid.lib"

-- Windows
Library["WinSock"] = "Ws2_32.lib"
Library["WinMM"] = "Winmm.lib"
Library["WinVersion"] = "Version.lib"
Library["BCrypt"] = "Bcrypt.lib"
