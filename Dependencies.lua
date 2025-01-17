
-- phx Dependencies

IncludeDir = {}
IncludeDir["ImGui"]         = "%{wks.location}/phoenix/vendor/ImGui"
IncludeDir["D3D12MA"]       = "%{wks.location}/phoenix/vendor/ImGui"
includeDir["AgilitySDK"]    = "%{wks.location}/vendor/agility_1.614.1/include"

LibraryDir = {}


Library = {}

-- Windows
Library["WinSock"] = "Ws2_32.lib"
Library["WinMM"] = "Winmm.lib"
Library["WinVersion"] = "Version.lib"
Library["BCrypt"] = "Bcrypt.lib"
