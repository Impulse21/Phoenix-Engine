cmake_minimum_required(VERSION 3.13)

# set the project name and version
project(PhxEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set_property( GLOBAL PROPERTY USE_FOLDERS ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP")
endif()

string(APPEND CMAKE_CXX_FLAGS_DEBUG " -D_DEBUG")

include(CMakeDependentOption)

add_subdirectory(Shaders)
set_target_properties(PhxEngineShaders PROPERTIES FOLDER PhxEngine)

add_subdirectory(ThirdParty)

#-- App
file(GLOB PhxEngineApp_Src
    Include/PhxEngine/App/*.h
    Src/App/*.cpp
)

#-- Core
file(GLOB PhxEngineCore_Src
    Include/PhxEngine/Core/*.h
    Src/Core/*.cpp
)

#-- Renderer
file(GLOB PhxEngineRenderer_Src
    Include/PhxEngine/Renderer/*.h
    Src/Renderer/*.h
    Src/Renderer/*.cpp
)

file(GLOB PhxEngineRenderer_ImGui_Src
    Src/Renderer/ImGui/*.h
    Src/Renderer/ImGui/*.cpp
)

#-- ECS
file(GLOB PhxEngineECS_Src
    Include/PhxEngine/ECS/*.h
    Src/ECS/*.h
    Src/ECS/*.cpp
)

#-- ImGui
file(GLOB PhxEngineImGui_Src
    Include/PhxEngine/ImGUi/*.h
    Src/ImGUi/*.h
    Src/ImGUi/*.cpp
)

#-- RHI
file(GLOB PhxEngineRHI_Src
    Include/PhxEngine/RHI/*.h)

#-- Dx12 implementaion
file(GLOB PhxEngineRHI_Dx12_Src
    Src/RHI/Dx12/*.h
    Src/RHI/Dx12/*.cpp
)

file (GLOB PhxEngineUtility_Src
    Include/PhxEngine/Utility/*.h
    Src/Utility/*.h
    Src/Utility/*.cpp)

add_library(PhxEngine STATIC EXCLUDE_FROM_ALL 
    ${PhxEngineApp_Src}
    ${PhxEngineCore_Src}
    ${PhxEngineRenderer_Src}
    ${PhxEngineRenderer_ImGui_Src}
    ${PhxEngineECS_Src}
    ${PhxEngineRHI_Src}
    ${PhxEngineRHI_Dx12_Src}
    ${PhxEngineImGui_Src}
    ${PhxEngineUtility_Src})


source_group("App"              FILES ${PhxEngineApp_Src})
source_group("Core"             FILES ${PhxEngineCore_Src})
source_group("Renderer"         FILES ${PhxEngineRenderer_Src})
source_group("Renderer/ImGui"   FILES ${PhxEngineRenderer_ImGui_Src})
source_group("ECS"              FILES ${PhxEngineECS_Src})
source_group("RHI"              FILES ${PhxEngineRHI_Src})
source_group("RHI/Dx12"         FILES ${PhxEngineRHI_Dx12_Src})
source_group("ImGUi"            FILES ${PhxEngineImGui_Src})
source_group("Utility"          FILES ${PhxEngineUtility_Src})


target_include_directories(PhxEngine PUBLIC Include  Shaders/Include)

target_link_libraries(PhxEngine
    spdlog
    glfw
    cgltf
    DirectXTex
    DirectX-Headers
    DirectX-Guids
    imgui
    d3d12.lib
    dxgi.lib
    PhxEngineShaders
    dxguid)

set_target_properties(PhxEngine PROPERTIES FOLDER PhxEngine)

if(WIN32)
    target_compile_definitions(PhxEngine PUBLIC NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()

#include(PhxEngineCore.cmake)
#include(PhxEngineRHI.cmake)
#include(PhxEngineRenderer.cmake)
#include(PhxEngineApp.cmake)

# -- Add Apps ---
add_subdirectory(TestBedApp)
add_subdirectory(Editor)