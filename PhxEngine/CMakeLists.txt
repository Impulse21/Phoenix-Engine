cmake_minimum_required(VERSION 3.16)


if (WIN32)
	# TODO: Choose whether to use SDL2 on windows as well
	set(TARGET_NAME PhxEngine_Windows)
else ()
	message( FATAL_ERROR "Only Windows is currently supported" )
endif()

project(${TARGET_NAME})

set(folder ${TARGET_NAME})

set_property( GLOBAL PROPERTY USE_FOLDERS ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP")
endif()

string(APPEND CMAKE_CXX_FLAGS_DEBUG " -D_DEBUG")

include(CMakeDependentOption)

set(extern_folder "${TARGET_NAME}/External Libraries")

file(GLOB PhxEngine_Engine_Src
    Engine/*.h
    Engine/*.cpp
)

file(GLOB PhxEngine_Core_Src
    Core/*.h
    Core/*.cpp
)

file(GLOB PhxEngine_Renderer_Src
	Renderer/*.h
	Renderer/*.cpp
)

file(GLOB PhxEngine_Assets_Src
    Assets/*.h
    Assets/*.cpp
)

file(GLOB PhxEngine_RHI_Src
    RHI/*.h
    RHI/*.cpp
)

file(GLOB PhxEngine_RHI_DX12_Src
    RHI/D3D12/*.h
    RHI/D3D12/*.cpp
)

file(GLOB PhxEngine_Graphics_ImGui_Src
    Renderer/ImGui/*.h
    Renderer/ImGui/*.cpp
)

file (GLOB PhxEngine_Inc
    *.h)
	
file (GLOB PhxEngine_Shader_Interop_Inc
	Shaders/*.h)


add_library(${TARGET_NAME} STATIC  
    ${PhxEngine_Engine_Src}
    ${PhxEngine_Core_Src}
    ${PhxEngine_Assets_Src}
    ${PhxEngine_RHI_Src}
    ${PhxEngine_RHI_DX12_Src}
    ${PhxEngine_Graphics_ImGui_Src}
	${PhxEngine_Renderer_Src}
	${PhxEngine_Shader_Interop_Inc}
    ${PhxEngine_Src})
	
add_library(PhxEngine ALIAS ${TARGET_NAME})

set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER 
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_include_directories(${TARGET_NAME} SYSTEM PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/RHI/D3D12
)

source_group("Engine"               FILES ${PhxEngine_Engine_Src})
source_group("Core"                 FILES ${PhxEngine_Core_Src})
source_group("RHI"					FILES ${PhxEngine_RHI_Src})
source_group("RHI/D3D12"			FILES ${PhxEngine_RHI_DX12_Src})
source_group("Renderer"             FILES ${PhxEngine_Renderer_Src})
source_group("Renderer/ImGui"       FILES ${PhxEngine_Graphics_ImGui_Src})
source_group("Assets"				FILES ${PhxEngine_Assets_Src})

add_subdirectory(Extern)

target_link_libraries(${TARGET_NAME} PUBLIC
	glfw
	DirectX-Headers
	DirectX-Guids
	cgltf
	DirectXTex
	DirectXMesh
	imgui
	stb
	entt
	cereal
	rapidjson
	tlsf
	D3D12MemoryAllocator
	mimalloc-static
	taskflow
	enkiTS)
    
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER ${folder})

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
target_link_libraries(${TARGET_NAME} PUBLIC D3D12AgilitySDK)

# Copy DLLs
add_custom_command(
	TARGET ${TARGET_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${d3d12-agility-sdk_SOURCE_DIR}/build/native/bin/x64"

	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/D3D12
	DEPENDS D3D12AgilitySDK)

message("D3D12 Agility SDK fetched")

# D3D12 Storage SDK
message("Fetching D3D12 Storage SDK")
FetchContent_Declare(d3d12-storage-sdk URL https://www.nuget.org/api/v2/package/Microsoft.Direct3D.DirectStorage/1.2.1)
FetchContent_MakeAvailable(d3d12-storage-sdk)
file(GLOB_RECURSE d3d12-storage-sdk_include "${d3d12-storage-sdk_SOURCE_DIR}/native/include/*.h")
add_library(
	D3D12StorageSDK INTERFACE
	${d3d12-storage-sdk_include}
	"${d3d12-storage-sdk_SOURCE_DIR}/native/bin/x64/dstorage.dll"
	"${d3d12-storage-sdk_SOURCE_DIR}/native/bin/x64/dstoragecore.dll")
target_include_directories(D3D12StorageSDK INTERFACE "${d3d12-storage-sdk_SOURCE_DIR}/native/include")
set_target_properties(D3D12StorageSDK  PROPERTIES FOLDER ${extern_folder})
target_link_libraries(${TARGET_NAME} PUBLIC D3D12StorageSDK)

# Copy DLLs
add_custom_command(
	TARGET ${TARGET_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${d3d12-storage-sdk_SOURCE_DIR}/native/bin/x64"

	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/D3D12
	DEPENDS D3D12StorageSDK)

message("D3D12 Storage SDK fetched")

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
target_link_libraries(${TARGET_NAME} PUBLIC DirectXShaderCompiler)

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
target_link_libraries(${TARGET_NAME} PUBLIC PIX)

# Copy DLLs
add_custom_command(
	TARGET ${TARGET_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	"${pix_SOURCE_DIR}/bin/x64/WinPixEventRuntime.dll"

	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
	DEPENDS PIX)
message("PIX fetched")