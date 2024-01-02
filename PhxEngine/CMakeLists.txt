cmake_minimum_required(VERSION 3.16)

# set the project name and version
set(project PhxEngine)

project(${project})

set(folder "PhxEngine")

set_property( GLOBAL PROPERTY USE_FOLDERS ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP")
endif()

string(APPEND CMAKE_CXX_FLAGS_DEBUG " -D_DEBUG")

include(CMakeDependentOption)

set(extern_folder "PhxEngine/External Libraries")

#-- App
file(GLOB PhxEngine_Engine_Inc
	Include/PhxEngine/Engine/*.h
)

file(GLOB PhxEngine_Engine_Src
    Src/Engine/*.h
    Src/Engine/*.cpp
)

#-- Core
file(GLOB PhxEngine_Core_Inc
	Include/PhxEngine/Core/*.h
)

file(GLOB PhxEngine_Core_Src
    Src/Core/*.h
    Src/Core/*.cpp
)

#-- Graphics
file(GLOB PhxEngine_Graphics_Inc
	Include/PhxEngine/Graphics/*.h
)

file(GLOB PhxEngine_Graphics_Src
    Src/Graphics/*.h
    Src/Graphics/*.cpp
)

#-- Renderer
file(GLOB PhxEngine_Renderer_Inc
	Include/PhxEngine/Renderer/*.h
)

file(GLOB PhxEngine_Renderer_Src
	Src/Renderer/*.h
	Src/Renderer/*.cpp
)

#-- Assets
file(GLOB PhxEngine_Assets_Inc
	Include/PhxEngine/Assets/*.h
)

file(GLOB PhxEngine_Assets_Src
    Src/Assets/*.h
    Src/Assets/*.cpp
)

#-- RHI
file(GLOB PhxEngine_Graphics_RHI_Inc
	Include/PhxEngine/RHI/*.h
)

file(GLOB PhxEngine_Graphics_RHI_Src
    Src/RHI/*.h
    Src/RHI/*.cpp
)

file(GLOB PhxEngine_Graphics_RHI_Windows_Src
    Src/RHI/Platform/Windows/*.h
    Src/RHI/Platform/Windows/*.cpp
)

file(GLOB PhxEngine_Graphics_RHI_DX12_Src
    Src/RHI/D3D12/*.h
    Src/RHI/D3D12/*.cpp
)

file(GLOB PhxEngine_Graphics_ImGui_Src
    Src/Renderer/ImGui/*.h
    Src/Renderer/ImGui/*.cpp
)

file (GLOB PhxEngine_Inc
    Include/PhxEngine/*.h)
	
file (GLOB PhxEngine_Src
    Src/*.h
	Src/*.cpp)
	
file (GLOB PhxEngine_Shader_Interop_Inc
	Include/PhxEngine/Shaders/*.h)



add_library(PhxEngine STATIC EXCLUDE_FROM_ALL  
    ${PhxEngine_Engine_Inc}
    ${PhxEngine_Engine_Src}
    ${PhxEngine_Core_Inc}
    ${PhxEngine_Core_Src}
    ${PhxEngine_Graphics_Inc}
    ${PhxEngine_Graphics_Src}
    ${PhxEngine_Assets_Inc}
    ${PhxEngine_Assets_Src}
    ${PhxEngine_Graphics_RHI_Inc}
    ${PhxEngine_Graphics_RHI_Src}
	${PhxEngine_Graphics_RHI_Windows_Src}
    ${PhxEngine_Graphics_RHI_DX12_Src}
    ${PhxEngine_Graphics_ImGui_Inc}
    ${PhxEngine_Graphics_ImGui_Src}
	${PhxEngine_Renderer_Inc}
	${PhxEngine_Renderer_Src}
	${PhxEngine_Renderer_DX12_Src}
	${PhxEngine_Shader_Interop_Inc}
    ${PhxEngine_Inc}
    ${PhxEngine_Src})

source_group("Include"					FILES ${PhxEngine_Inc})
source_group("Include/Engine"           FILES ${PhxEngine_Engine_Inc})
source_group("Include/Core"             FILES ${PhxEngine_Core_Inc})
source_group("Include/Graphics"         FILES ${PhxEngine_Graphics_Inc})
source_group("Include/RHI"				FILES ${PhxEngine_Graphics_RHI_Inc})
source_group("Include/Graphics/ImGui"   FILES ${PhxEngine_Graphics_ImGui_Inc})
source_group("Include/Graphics"         FILES ${PhxEngine_Graphics_Inc})
source_group("Include/System"           FILES ${PhxEngine_System_Inc})
source_group("Include/Renderer"			FILES ${PhxEngine_Renderer_Inc})
source_group("Include/Interop"			FILES ${PhxEngine_Shader_Interop_Inc})
source_group("Include/Assets"			FILES ${PhxEngine_Assets_Inc})

source_group("Src"						FILES ${PhxEngine_Src})
source_group("Src/Engine"               FILES ${PhxEngine_Engine_Src})
source_group("Src/Core"                 FILES ${PhxEngine_Core_Src})
source_group("Src/Graphics"             FILES ${PhxEngine_Graphics_Src})
source_group("Src/RHI"					FILES ${PhxEngine_Graphics_RHI_Src})
source_group("Src/RHI/Platform/Windows"	FILES ${PhxEngine_Graphics_RHI_Windows_Src})
source_group("Src/RHI/D3D12"			FILES ${PhxEngine_Graphics_RHI_DX12_Src})
source_group("Src/Renderer"             FILES ${PhxEngine_Renderer_Src})
source_group("Src/Renderer/ImGui"       FILES ${PhxEngine_Graphics_ImGui_Src})
source_group("Src/Assets"				FILES ${PhxEngine_Assets_Src})


#add_subdirectory(Shaders)

#add_dependencies(${project} ${project}_shaders)
target_include_directories(${project} 
	PUBLIC  "Include"
	PRIVATE "Src")

# target_precompile_headers(${project} PRIVATE Src/phxpch.h)

add_subdirectory(Extern)

target_link_libraries(${project} PUBLIC
	glfw
	DirectX-Headers
	DirectX-Guids
	cgltf
	DirectXTex
	meshoptimizer
	imgui
	stb
	entt
	json
	tlsf
	D3D12MemoryAllocator
	mimalloc-static
	taskflow
	enkiTS
	ankerl)

set_target_properties(${project} PROPERTIES FOLDER ${folder})

#INCLUDE_EXTERNAL_MSPROJECT(ShadersSource ${CMAKE_SOURCE_DIR}/PhxEngine/Shaders/ShadersSource.vcitems)

# D3D12 Agility SDK
message("Fetching D3D12 Agility SDK")
FetchContent_Declare(d3d12-agility-sdk URL https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.608.2)
FetchContent_MakeAvailable(d3d12-agility-sdk)
file(GLOB_RECURSE d3d12-agility-sdk_include "${d3d12-agility-sdk_SOURCE_DIR}/build/native/include/*.h")
add_library(
	D3D12AgilitySDK INTERFACE
	${d3d12-agility-sdk_include}
	"${d3d12-agility-sdk_SOURCE_DIR}/build/native/bin/x64/D3D12Core.dll"
	"${d3d12-agility-sdk_SOURCE_DIR}/build/native/bin/x64/d3d12SDKLayers.dll")
target_include_directories(D3D12AgilitySDK INTERFACE "${d3d12-agility-sdk_SOURCE_DIR}/build/native/include")
target_link_libraries(D3D12AgilitySDK INTERFACE "d3d12.lib" "dxgi.lib")
set_target_properties(D3D12AgilitySDK  PROPERTIES FOLDER ${extern_folder})
target_link_libraries(${project} PUBLIC D3D12AgilitySDK)

# Copy DLLs
add_custom_command(
	TARGET ${project} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${d3d12-agility-sdk_SOURCE_DIR}/build/native/bin/x64"

	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/D3D12
	DEPENDS D3D12AgilitySDK)

message("D3D12 Agility SDK fetched")

# DXC
message("Fetching DirectXShaderCompiler")
FetchContent_Declare(dxc URL https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.7.2308/dxc_2023_08_14.zip )
FetchContent_MakeAvailable(dxc)
file(MAKE_DIRECTORY ${dxc_SOURCE_DIR}/include)
file(COPY ${dxc_SOURCE_DIR}/inc/d3d12shader.h DESTINATION ${dxc_SOURCE_DIR}/include/dxc)
file(COPY ${dxc_SOURCE_DIR}/inc/dxcapi.h DESTINATION ${dxc_SOURCE_DIR}/include/dxc)
add_library(
	DirectXShaderCompiler
	INTERFACE
	"${dxc_SOURCE_DIR}/include/dxc/d3d12shader.h"
	"${dxc_SOURCE_DIR}/include/dxc/dxcapi.h"
	"${dxc_SOURCE_DIR}/bin/x64/dxcompiler.dll"
	"${dxc_SOURCE_DIR}/bin/x64/dxil.dll")
target_include_directories(DirectXShaderCompiler INTERFACE "${dxc_SOURCE_DIR}/include")
target_link_libraries(DirectXShaderCompiler INTERFACE "${dxc_SOURCE_DIR}/lib/x64/dxcompiler.lib")
set_target_properties(DirectXShaderCompiler  PROPERTIES FOLDER ${extern_folder})
target_link_libraries(${project} PUBLIC DirectXShaderCompiler)

# Copy DLLs
#add_custom_command(
#	TARGET ${project} POST_BUILD
#	COMMAND ${CMAKE_COMMAND} -E copy
#	"${dxc_SOURCE_DIR}/bin/x64/dxcompiler.dll"
#	"${dxc_SOURCE_DIR}/bin/x64/dxil.dll"
#
#	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>
#	DEPENDS DirectXShaderCompiler)
message("DirectXShaderCompiler fetched")

# PIX
message("Fetching PIX")
FetchContent_Declare(pix URL https://www.nuget.org/api/v2/package/WinPixEventRuntime/1.0.220124001)
FetchContent_MakeAvailable(pix)
file(GLOB_RECURSE pix_include "${pix_SOURCE_DIR}/Include/WinPixEventRuntime/*.h")
add_library(
	PIX INTERFACE
	${pix_include}
	"${pix_SOURCE_DIR}/bin/x64/WinPixEventRuntime.dll")
target_include_directories(PIX INTERFACE "${pix_SOURCE_DIR}/Include/WinPixEventRuntime")
target_link_libraries(PIX INTERFACE "${pix_SOURCE_DIR}/bin/x64/WinPixEventRuntime.lib")
set_target_properties(PIX  PROPERTIES FOLDER ${extern_folder})
target_link_libraries(${project} PUBLIC PIX)

# Copy DLLs
add_custom_command(
	TARGET ${project} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	"${pix_SOURCE_DIR}/bin/x64/WinPixEventRuntime.dll"

	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
	DEPENDS PIX)
message("PIX fetched")
