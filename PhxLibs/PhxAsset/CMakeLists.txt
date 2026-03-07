# PhxAsset Library

file(GLOB_RECURSE PHXASSET_SOURCES
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

file(GLOB_RECURSE PHXASSET_HEADERS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/PhxAsset/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/PhxAsset/*.hpp"
)

add_library(PhxAsset STATIC
    ${PHXASSET_SOURCES}
    ${PHXASSET_HEADERS}
)

target_include_directories(PhxAsset
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(PhxAsset
    PUBLIC
        PhxCore
    PRIVATE
        yaml-cpp::yaml-cpp
)


if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/PhxAsset_pch.h")
    target_precompile_headers(PhxAsset PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/PhxAsset_pch.h"
    )
endif()


# Visual Studio organization
set_target_properties(PhxAsset PROPERTIES FOLDER "PhxLibs")

# Apply source filters for Visual Studio
if(COMMAND apply_filters)
    apply_filters(PhxAsset)
endif()