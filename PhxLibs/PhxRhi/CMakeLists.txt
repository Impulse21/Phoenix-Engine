# PhxRhi Library (Rendering Hardware Interface)

file(GLOB_RECURSE PHXRHI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

file(GLOB_RECURSE PHXRHI_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/PhxRhi/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/PhxRhi/*.hpp"
)

add_library(PhxRhi STATIC
    ${PHXRHI_SOURCES}
    ${PHXRHI_HEADERS}
)

target_include_directories(PhxRhi
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan  # Vulkan backend
)

target_link_libraries(PhxRhi
    PUBLIC
        PhxCore
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
        volk::volk
        vk-bootstrap::vk-bootstrap
        slang
    PRIVATE
)

target_compile_definitions(PhxRhi
    PUBLIC
        PHX_RHI_VULKAN
)

# Vulkan validation layers for debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(PhxRhi PRIVATE
        VK_USE_VALIDATION_LAYERS
    )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/PhxRhi_pch.h")
    target_precompile_headers(PhxRhi PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/PhxRhi_pch.h"
    )
endif()