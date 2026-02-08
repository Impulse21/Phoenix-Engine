# PhxResource Library

file(GLOB_RECURSE PHXRESOURCE_SOURCES
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

file(GLOB_RECURSE PHXRESOURCE_HEADERS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/PhxResource/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/PhxResource/*.hpp"
)

add_library(PhxResource STATIC
    ${PHXRESOURCE_SOURCES}
    ${PHXRESOURCE_HEADERS}
)

target_include_directories(PhxResource
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(PhxResource
    PUBLIC
        PhxCore
        PhxRhi
        cgltf
        stb
        nlohmann_json::nlohmann_json
    PRIVATE
)


if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/PhxResource_pch.h")
    target_precompile_headers(PhxResource PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/PhxResource_pch.h"
    )
endif()


# Visual Studio organization
set_target_properties(PhxResource PROPERTIES FOLDER "PhxLibs")

# Apply source filters for Visual Studio
if(COMMAND apply_filters)
    apply_filters(PhxResource)
endif()