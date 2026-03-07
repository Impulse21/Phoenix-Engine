# PhxRenderer Library

file(GLOB_RECURSE PHXRENDERER_SOURCES
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

file(GLOB_RECURSE PHXRENDERER_HEADERS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/PhxRenderer/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/PhxRenderer/*.hpp"
)

add_library(PhxRenderer STATIC
    ${PHXRENDERER_SOURCES}
    ${PHXRENDERER_HEADERS}
)

target_include_directories(PhxRenderer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(PhxRenderer
    PUBLIC
        PhxCore
        PhxRhi
        PhxAsset
        PhxResource
        yaml-cpp
        meshoptimizer
        stb
        bc7enc_rdo
        ImGui
    PRIVATE
)


if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/PhxRenderer_pch.h")
    target_precompile_headers(PhxRenderer PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/PhxRenderer_pch.h"
    )
endif()


# Visual Studio organization
set_target_properties(PhxRenderer PROPERTIES FOLDER "PhxLibs")

# Apply source filters for Visual Studio
if(COMMAND apply_filters)
    apply_filters(PhxRenderer)
endif()