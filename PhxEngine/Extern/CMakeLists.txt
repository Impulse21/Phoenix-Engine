# Required dependencies
cmake_minimum_required(VERSION 3.16)

include(FetchContent)


set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

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

add_library(taskflow INTERFACE)
target_include_directories(taskflow INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/taskflow)

option(BUILD_TOOLS "" OFF)
option(BUILD_DX11 "" OFF)
option(BUILD_DX12 "" OFF)

add_subdirectory(DirectXTex)
set_target_properties(DirectXTex PROPERTIES FOLDER ${extern_folder})

add_subdirectory(DirectXMesh)
set_target_properties(DirectXMesh PROPERTIES FOLDER ${extern_folder})

add_subdirectory(meshoptimizer)
set_target_properties(meshoptimizer PROPERTIES FOLDER ${extern_folder})

add_subdirectory(D3D12MemoryAllocator)
set_target_properties(D3D12MemoryAllocator PROPERTIES FOLDER ${extern_folder})
 target_include_directories(D3D12MemoryAllocator INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/D3D12MemoryAllocator/include)

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

file (GLOB cereal_src
    ${CMAKE_CURRENT_SOURCE_DIR}/cereal/cereal/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cereal/cereal/archives/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cereal/cereal/details/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cereal/cereal/external/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cereal/cereal/types/*.h)
    
add_library(cereal INTERFACE ${cereal_src})
target_include_directories(cereal INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cereal)
set_target_properties(cereal PROPERTIES FOLDER ${extern_folder})

file (GLOB rapidjson_src
    ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/rapidjson/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/rapidjson/error/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/rapidjson/internal/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/rapidjson/msinttypes/*.h)
    
add_library(rapidjson INTERFACE ${rapidjson_src})
target_include_directories(rapidjson INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson)
set_target_properties(rapidjson PROPERTIES FOLDER ${extern_folder})

file (GLOB stb_src
    ${CMAKE_CURRENT_SOURCE_DIR}/stb/*.h)

add_library(stb INTERFACE ${stb_src})
set_target_properties(stb  PROPERTIES FOLDER ${extern_folder})

file (GLOB tlsf_src
    ${CMAKE_CURRENT_SOURCE_DIR}/tlsf/tlsf.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tlsf/tlsf.c)

add_library(tlsf STATIC ${CMAKE_CURRENT_SOURCE_DIR}/tlsf/tlsf.c)
target_include_directories(tlsf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tlsf)
set_target_properties(tlsf PROPERTIES FOLDER ${extern_folder})

set(MI_USE_CXX      ON)
set(MI_BUILD_SHARED OFF)
set(MI_BUILD_STATIC ON)
set(MI_BUILD_OBJECT OFF)
set(MI_BUILD_TESTS  OFF)
add_subdirectory(mimalloc)
set_target_properties(mimalloc-static PROPERTIES FOLDER ${extern_folder})

set( ENKITS_BUILD_C_INTERFACE  OFF )
set( ENKITS_BUILD_EXAMPLES     OFF )
set( ENKITS_BUILD_SHARED       OFF )
set( ENKITS_INSTALL            OFF )
add_subdirectory(enkiTS)
set_target_properties(enkiTS PROPERTIES FOLDER ${extern_folder})
