
-- phx Dependencies

IncludeDir = {}
IncludeDir["ImGui"] = "%{wks.location}/phoenix/vendor/ImGui"

LibraryDir = {}


Library = {}

-- Windows
Library["WinSock"] = "Ws2_32.lib"
Library["WinMM"] = "Winmm.lib"
Library["WinVersion"] = "Version.lib"
Library["BCrypt"] = "Bcrypt.lib"
