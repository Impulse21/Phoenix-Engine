
# Required dependencies
set(third_party_folder "ThirdParty Libraries")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include)

# GLFW Library
option(GLFW_BUILD_EXAMPLES "" OFF)
option(GLFW_BUILD_TESTS "" OFF)
option(GLFW_BUILD_DOCS "" OFF)
option(GLFW_INSTALL "" OFF)
add_subdirectory(glfw)

set_target_properties(glfw PROPERTIES FOLDER ${third_party_folder})


add_subdirectory(DirectX-Headers)

set_target_properties(DirectX-Guids PROPERTIES FOLDER ${third_party_folder})

add_library(cgltf INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/cgltf/cgltf.h)

target_include_directories(cgltf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cgltf)
    
set_target_properties(cgltf PROPERTIES FOLDER ${third_party_folder})

option(BUILD_TOOLS "" OFF)
option(BUILD_DX11 "" OFF)
option(BUILD_DX12 "" OFF)

add_subdirectory(DirectXTex)
set_target_properties(DirectXTex PROPERTIES FOLDER ${third_party_folder})
