# Required dependencies
cmake_minimum_required(VERSION 3.16)

include(FetchContent)


set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include)

# GLFW Library
option(GLFW_BUILD_EXAMPLES "" OFF)
option(GLFW_BUILD_TESTS "" OFF)
option(GLFW_BUILD_DOCS "" OFF)
option(GLFW_INSTALL "" OFF)
add_subdirectory(glfw)

set_target_properties(glfw PROPERTIES FOLDER ${extern_folder})
set_target_properties(update_mappings PROPERTIES FOLDER ${extern_folder})

add_subdirectory(DirectX-Headers)

set_target_properties(DirectX-Guids PROPERTIES FOLDER ${extern_folder})

add_library(cgltf INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/cgltf/cgltf.h)

target_include_directories(cgltf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cgltf)
    
set_target_properties(cgltf PROPERTIES FOLDER ${extern_folder})

add_library(entt INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/entt/entt.hpp)
    
target_include_directories(entt INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/entt)
    
set_target_properties(entt PROPERTIES FOLDER ${extern_folder})

option(BUILD_TOOLS "" OFF)
option(BUILD_DX11 "" OFF)
option(BUILD_DX12 "" OFF)

add_subdirectory(DirectXTex)
set_target_properties(DirectXTex PROPERTIES FOLDER ${extern_folder})

set(imgui_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imconfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_internal.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_rectpack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_textedit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_truetype.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/misc/cpp/imgui_stdlib.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/misc/cpp/imgui_stdlib.cpp)

add_library(imgui STATIC ${imgui_SRC})
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
set_target_properties(imgui PROPERTIES FOLDER ${extern_folder})



file (GLOB stb_src
    ${CMAKE_CURRENT_SOURCE_DIR}/stb/*.h)

add_library(stb INTERFACE ${stb_src})
set_target_properties(stb  PROPERTIES FOLDER ${extern_folder})