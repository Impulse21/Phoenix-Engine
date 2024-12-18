set(extern_folder 3rdParty)

add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/spdlog)

add_library(cgltf INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/cgltf/cgltf.h)
target_include_directories(cgltf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cgltf)
set_target_properties(cgltf PROPERTIES FOLDER ${extern_folder})

add_library(entt INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/entt/entt.hpp)
target_include_directories(entt INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/entt)
set_target_properties(entt PROPERTIES FOLDER ${extern_folder})

set(imgui_src
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

add_library(imgui STATIC ${imgui_src})
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
set_target_properties(imgui PROPERTIES FOLDER ${extern_folder})

option(BUILD_TOOLS "" OFF)
option(BUILD_DX11 "" OFF)
option(BUILD_DX12 "" OFF)

add_subdirectory(DirectXTex)
set_target_properties(DirectXTex PROPERTIES FOLDER ${extern_folder})


add_subdirectory(D3D12MemoryAllocator)
set_target_properties(D3D12MemoryAllocator PROPERTIES FOLDER ${extern_folder})
 target_include_directories(D3D12MemoryAllocator INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/D3D12MemoryAllocator/include)