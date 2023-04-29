include(FetchContent)
include(../compileshaders.cmake)

set(folder "PhxEngine")

file (GLOB PhxEngine_Shader_Inc
	../Include/PhxEngine/Shaders/*.h
	*.hlsli)

file (GLOB PhxEngine_Shader_VS_Src
	*VS.hlsl)

file (GLOB PhxEngine_Shader_PS_Src
	*PS.hlsl)

file (GLOB PhxEngine_Shader_CS_Src
	*CS.hlsl)

file (GLOB PhxEngine_Shader_AS_Src
	*AS.hlsl)

file (GLOB PhxEngine_Shader_MS_Src
	*MS.hlsl)
	
 set (PhxEngine_Shaders
	${PhxEngine_Shader_VS_Src}
	${PhxEngine_Shader_PS_Src}
	${PhxEngine_Shader_CS_Src}
	${PhxEngine_Shader_AS_Src}
	${PhxEngine_Shader_MS_Src})


source_group("Shaders/Include"		    FILES ${PhxEngine_Shader_Inc})
source_group("Shaders/VS"			    FILES ${PhxEngine_Shader_VS_Src})
source_group("Shaders/PS"			    FILES ${PhxEngine_Shader_PS_Src})
source_group("Shaders/CS"			    FILES ${PhxEngine_Shader_CS_Src})
source_group("Shaders/AS"			    FILES ${PhxEngine_Shader_AS_Src})
source_group("Shaders/MS"			    FILES ${PhxEngine_Shader_MS_Src})

set_source_files_properties(${PhxEngine_Shader_Interop_Inc} PROPERTIES VS_TOOL_OVERRIDE "None")
set_source_files_properties(${PhxEngine_Shader_Inc} PROPERTIES VS_TOOL_OVERRIDE "None")
set_source_files_properties(${PhxEngine_Shader_VS_Src} PROPERTIES VS_TOOL_OVERRIDE "None" ShaderType "vs" ShaderModel "6_6" EntryPoint "main")
set_source_files_properties(${PhxEngine_Shader_PS_Src} PROPERTIES VS_TOOL_OVERRIDE "None" ShaderType "ps" ShaderModel "6_6" EntryPoint "main")
set_source_files_properties(${PhxEngine_Shader_CS_Src} PROPERTIES VS_TOOL_OVERRIDE "None" ShaderType "cs" ShaderModel "6_6" EntryPoint "main")
set_source_files_properties(${PhxEngine_Shader_AS_Src} PROPERTIES VS_TOOL_OVERRIDE "None" ShaderType "as" ShaderModel "6_6" EntryPoint "main")
set_source_files_properties(${PhxEngine_Shader_MS_Src} PROPERTIES VS_TOOL_OVERRIDE "None" ShaderType "ms" ShaderModel "6_6" EntryPoint "main")

message("Fetching DirectXShaderCompiler")
FetchContent_Declare(dxc URL https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.7.2212.1/dxc_2023_03_01.zip)
FetchContent_MakeAvailable(dxc)

dxc_compile_shaders(
    TARGET ${project}_shaders
    SOURCES ${PhxEngine_Shaders}
    CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/shaders.cfg
    FOLDER ${folder}
    OUTPUT_BASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Shaders/${project}
	INCLUDES ../Include/PhxEngine/Shaders/
)

target_sources(${project}_shaders PRIVATE ${PhxEngine_Shader_Inc})