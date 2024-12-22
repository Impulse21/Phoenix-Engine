-- Functionality that is common to all lua scripts in the Corsair Engine

function GetCopyLibraryPostBuildCommand(action)

	local post_build_copy_command = "{copy} %{cfg.buildtarget.abspath} ../../Binaries/%{prj.name}."..action..".%{cfg.buildcfg:lower()}".."%{cfg.buildtarget.extension}"

	if action == "vs2017" then
		post_build_copy_command = post_build_copy_command.."*"
	end
	
	return post_build_copy_command
end