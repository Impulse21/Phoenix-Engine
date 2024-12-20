set(3rdparty_folder 3rdParty)

add_subdirectory(spdlog)
set_target_properties(spdlog PROPERTIES FOLDER ${3rdparty_folder})

add_library(cgltf INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/cgltf/cgltf.h)
target_include_directories(cgltf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cgltf)
set_target_properties(cgltf PROPERTIES FOLDER ${3rdparty_folder})

add_library(entt INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/entt/src/entt/entt.hpp)
target_include_directories(entt SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/entt)
set_target_properties(entt PROPERTIES FOLDER ${3rdparty_folder})

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
target_include_directories(imgui SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
set_target_properties(imgui PROPERTIES FOLDER ${3rdparty_folder})

option(BUILD_TOOLS "" OFF)
option(BUILD_DX11 "" OFF)
option(BUILD_DX12 "" OFF)

add_subdirectory(DirectXTex)
set_target_properties(DirectXTex PROPERTIES FOLDER ${3rdparty_folder})


message("Fetching D3D12 Agility SDK")
set(D3D12_AGILITY_SDK_VERSION "1.715.1-preview")
string(CONFIGURE "https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/${D3D12_AGILITY_SDK_VERSION}" D3D12_AGILITY_SDK_URL)

FetchContent_Declare(
    d3d12-agility-sdk 
    URL ${D3D12_AGILITY_SDK_URL}
    SOURCE_DIR ${CMAKE_BINARY_DIR}/d3d12-agility-sdk)

FetchContent_MakeAvailable(d3d12-agility-sdk)

add_library(AgilitySDK SHARED IMPORTED)
set_target_properties(AgilitySDK PROPERTIES
    IMPORTED_LOCATION "${d3d12-agility-sdk_SOURCE_DIR}/bin/D3D12Core.dll"
    IMPORTED_IMPLIB "${d3d12-agility-sdk_SOURCE_DIR}/lib/D3D12Core.lib"
)

set(d3d12-agility-sdk_include "${d3d12-agility-sdk_SOURCE_DIR}/build/native/include/")
message("D3D12 Agility SDK fetched")

set(D3D12MA_AGILITY_SDK_DIRECTORY ${d3d12-agility-sdk_SOURCE_DIR})

message(STATUS "Agility SDK SRC path: ${d3d12-agility-sdk_SOURCE_DIR}")
message(STATUS "Agility SDK include path: ${d3d12-agility-sdk_include}")

add_subdirectory(D3D12MemoryAllocator)
set_target_properties(D3D12MemoryAllocator PROPERTIES FOLDER ${3rdparty_folder})

target_compile_options(D3D12MemoryAllocator PRIVATE 
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-error>
)