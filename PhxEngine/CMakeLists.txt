cmake_minimum_required(VERSION 3.16)

# set the project name and version
set(ProjectName PhxEngine)

project(${ProjectName})


set_property( GLOBAL PROPERTY USE_FOLDERS ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP")
endif()

string(APPEND CMAKE_CXX_FLAGS_DEBUG " -D_DEBUG")

include(CMakeDependentOption)

#add_subdirectory(Shaders)
#set_target_properties(PhxEngineShaders PROPERTIES FOLDER PhxEngine)

set(extern_folder "PhxEngine/External Libraries")

#-- App

file(GLOB PhxEngine_App_Inc
	Include/PhxEngine/App/*.h
)

file(GLOB PhxEngine_App_Src
    Src/App/*.h
    Src/App/*.cpp
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

#-- Graphics RHI
file(GLOB PhxEngine_Graphics_RHI_Inc
	Include/PhxEngine/Graphics/RHI/*.h
)

file(GLOB PhxEngine_Graphics_RHI_DX12_Src
    Src/Graphics/RHI/Dx12/*.h
    Src/Graphics/RHI/Dx12/*.cpp
)

#-- Graphics IMGUI
file(GLOB PhxEngine_Graphics_ImGui_Inc
	Include/PhxEngine/Graphics/ImGui/*.h
)

file(GLOB PhxEngine_Graphics_ImGui_Src
    Src/Graphics/ImGui/*.h
    Src/Graphics/ImGui/*.cpp
)

#-- System
file(GLOB PhxEngine_System_Inc
	Include/PhxEngine/Systems/*.h
)

file(GLOB PhxEngine_System_Src
    Src/Systems/*.h
    Src/Systems/*.cpp
)

#-- Scene
file (GLOB PhxEngine_Scene_Inc
	Include/PhxEngine/Scene/*.h)

file (GLOB PhxEngine_Scene_Src
    Src/Scene/*.h
    Src/Scene/*.cpp)

file (GLOB PhxEngine_Inc
    Include/PhxEngine/*.h)
	
file (GLOB PhxEngine_Src
    Src/*.h
	Src/*.cpp)

add_library(PhxEngine STATIC EXCLUDE_FROM_ALL  
    ${PhxEngine_App_Inc}
    ${PhxEngine_App_Src}
    ${PhxEngine_Core_Inc}
    ${PhxEngine_Core_Src}
    ${PhxEngine_Graphics_Inc}
    ${PhxEngine_Graphics_Src}
    ${PhxEngine_Graphics_RHI_Inc}
    ${PhxEngine_Graphics_RHI_Src}
    ${PhxEngine_Graphics_RHI_DX12_Src}
    ${PhxEngine_Graphics_ImGui_Inc}
    ${PhxEngine_Graphics_ImGui_Src}
    ${PhxEngine_System_Inc}
    ${PhxEngine_System_Src}
    ${PhxEngine_Scene_Inc}
    ${PhxEngine_Scene_Src}
    ${PhxEngine_Inc}
    ${PhxEngine_Src})

source_group("Include"					FILES ${PhxEngine_Inc})
source_group("Include/App"              FILES ${PhxEngine_App_Inc})
source_group("Include/Core"             FILES ${PhxEngine_Core_Inc})
source_group("Include/Graphics"         FILES ${PhxEngine_Graphics_Inc})
source_group("Include/Graphics/RHI"     FILES ${PhxEngine_Graphics_RHI_Inc})
source_group("Include/Graphics/ImGui"   FILES ${PhxEngine_Graphics_ImGui_Inc})
source_group("Include/System"           FILES ${PhxEngine_System_Inc})
source_group("Include/Scene"            FILES ${PhxEngine_Scene_Inc})

source_group("Src"						FILES ${PhxEngine_Src})
source_group("Src/App"                  FILES ${PhxEngine_App_Src})
source_group("Src/Core"                 FILES ${PhxEngine_Core_Src})
source_group("Src/Graphics"             FILES ${PhxEngine_Graphics_Src})
source_group("Src/Graphics/RHI"         FILES ${PhxEngine_Graphics_RHI_Src})
source_group("Src/Graphics/RHI/Dx12"    FILES ${PhxEngine_Graphics_RHI_DX12_Src})
source_group("Src/Graphics/ImGui"       FILES ${PhxEngine_Graphics_ImGui_Src})
source_group("Src/System"               FILES ${PhxEngine_System_Src})
source_group("Src/Scene"                FILES ${PhxEngine_Scene_Src})

target_include_directories(${ProjectName} PUBLIC Include ../Shaders/Include)
target_include_directories(${ProjectName} PRIVATE Src ../Shaders/Include)

target_precompile_headers(${ProjectName} PRIVATE Src/phxpch.h)

add_subdirectory(Extern)

target_link_libraries(${ProjectName} PUBLIC
	spdlog
	glfw
	DirectX-Headers
	DirectX-Guids
	cgltf
	DirectXTex
	imgui
	stb
	entt
	cereal
	rapidjson)
    
set_target_properties(${ProjectName} PROPERTIES FOLDER PhxEngine)

if(WIN32)
    target_compile_definitions(${ProjectName} PUBLIC NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()

# D3D12 Agility SDK
message("Fetching D3D12 Agility SDK")
FetchContent_Declare(d3d12-agility-sdk URL https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.606.3)
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
target_link_libraries(${ProjectName} PUBLIC D3D12AgilitySDK)

# Copy DLLs
add_custom_command(
	TARGET ${ProjectName} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${d3d12-agility-sdk_SOURCE_DIR}/build/native/bin/x64"

	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/D3D12
	DEPENDS D3D12AgilitySDK)

message("D3D12 Agility SDK fetched")

# DXC
message("Fetching DirectXShaderCompiler")
FetchContent_Declare(dxc URL https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.7.2207/dxc_2022_07_18.zip)
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
target_link_libraries(${ProjectName} PUBLIC DirectXShaderCompiler)

# Copy DLLs
add_custom_command(
	TARGET ${ProjectName} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	"${dxc_SOURCE_DIR}/bin/x64/dxcompiler.dll"
	"${dxc_SOURCE_DIR}/bin/x64/dxil.dll"

	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>
	DEPENDS DirectXShaderCompiler)
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
target_link_libraries(${ProjectName} PUBLIC PIX)

# Copy DLLs
add_custom_command(
	TARGET ${ProjectName} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	"${pix_SOURCE_DIR}/bin/x64/WinPixEventRuntime.dll"

	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>
	DEPENDS PIX)
message("PIX fetched")