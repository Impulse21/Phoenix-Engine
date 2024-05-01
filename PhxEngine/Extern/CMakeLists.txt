cmake_minimum_required(VERSION 3.16)


set(OPTICK_USE_D3D12 ON)
add_subdirectory(optick)
set_target_properties(OptickCore PROPERTIES FOLDER ${extern_folder})

file (GLOB nlohmann_json_src
    ${CMAKE_CURRENT_SOURCE_DIR}/nlohmann_json/*.hpp)

add_library(json INTERFACE ${nlohmann_json_src})
target_include_directories(json INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/nlohmann_json" )
set_target_properties(json PROPERTIES FOLDER ${extern_folder})


file (GLOB CmdParser
    ${CMAKE_CURRENT_SOURCE_DIR}/CmdParser/*.hpp)

add_library(CmdParser INTERFACE ${CmdParser})
target_include_directories(CmdParser INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/CmdParser" )
set_target_properties(CmdParser PROPERTIES FOLDER ${extern_folder})

# GLFW Library
option(GLFW_BUILD_EXAMPLES "" OFF)
option(GLFW_BUILD_TESTS "" OFF)
option(GLFW_BUILD_DOCS "" OFF)
option(GLFW_INSTALL "" OFF)
add_subdirectory(glfw)

set_target_properties(glfw PROPERTIES FOLDER ${extern_folder})
set_target_properties(update_mappings PROPERTIES FOLDER ${extern_folder})

add_subdirectory(D3D12MemoryAllocator)
set_target_properties(D3D12MemoryAllocator PROPERTIES FOLDER ${extern_folder})
 target_include_directories(D3D12MemoryAllocator INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/D3D12MemoryAllocator/include)

 
add_subdirectory(DirectX-Headers)
set_target_properties(DirectX-Headers PROPERTIES FOLDER ${extern_folder})
set_target_properties(DirectX-Guids PROPERTIES FOLDER ${extern_folder})

add_library(entt INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/entt/entt.hpp)
target_include_directories(entt INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/entt)
set_target_properties(entt PROPERTIES FOLDER ${extern_folder})


add_library(taskflow INTERFACE)
target_include_directories(taskflow INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/taskflow)
set_target_properties(taskflow PROPERTIES FOLDER ${extern_folder})

add_subdirectory(yaml-cpp)
set_target_properties(yaml-cpp PROPERTIES FOLDER ${extern_folder})
set_target_properties(yaml-cpp-sandbox PROPERTIES FOLDER ${extern_folder})
set_target_properties(yaml-cpp-parse PROPERTIES FOLDER ${extern_folder})
set_target_properties(yaml-cpp-read PROPERTIES FOLDER ${extern_folder})

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
