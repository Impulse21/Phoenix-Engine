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
file(GLOB PhxEngine_App_Src
    Src/App/*.h
    Src/App/*.cpp
)

#-- Core
file(GLOB PhxEngine_Core_Src
    Src/Core/*.h
    Src/Core/*.cpp
)

#-- Graphics
file(GLOB PhxEngine_Graphics_Src
    Src/Graphics/*.h
    Src/Graphics/*.cpp
)

#-- Graphics RHI
file(GLOB PhxEngine_Graphics_RHI_Src
    Src/Graphics/RHI/*.h
)

file(GLOB PhxEngine_Graphics_RHI_DX12_Src
    Src/Graphics/RHI/Dx12/*.h
    Src/Graphics/RHI/Dx12/*.cpp
)

#-- Graphics IMGUI
file(GLOB PhxEngine_Graphics_ImGui_Src
    Src/Graphics/ImGui/*.h
    Src/Graphics/ImGui/*.cpp
)

#-- System
file(GLOB PhxEngine_System_Src
    Src/System/*.h
    Src/System/*.cpp
)
#-- Scene
file (GLOB PhxEngine_Scene_Src
    Src/Scene/*.h
    Src/Scene/*.cpp)

add_library(PhxEngine STATIC EXCLUDE_FROM_ALL 
    ${PhxEngine_App_Src}
    ${PhxEngine_Core_Src}
    ${PhxEngine_Graphics_Src}
    ${PhxEngine_Graphics_RHI_Src}
    ${PhxEngine_Graphics_RHI_DX12_Src}
    ${PhxEngine_Graphics_ImGui_Src}
    ${PhxEngine_System_Src}
    ${PhxEngine_Scene_Src})


source_group("App"                  FILES ${PhxEngine_App_Src})
source_group("Core"                 FILES ${PhxEngine_Core_Src})
source_group("Graphics"             FILES ${PhxEngine_Graphics_Src})
source_group("Graphics/RHI"         FILES ${PhxEngine_RHI_Src})
source_group("Graphics/RHI/Dx12"    FILES ${PhxEngine_RHI_Dx12_Src})
source_group("Graphics/ImGui"       FILES ${PhxEngine_Graphics_ImGui_Src})
source_group("System"               FILES ${PhxEngine_System_Src})
source_group("Scene"                FILES ${PhxEngine_Scene_Src})

target_include_directories(PhxEngine PUBLIC Include  ../Shaders/Include)

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

set_property(TARGET PhxEngine
    PROPERTY VS_PACKAGE_REFERENCES "Microsoft.Direct3D.D3D12_1.606.3"
)