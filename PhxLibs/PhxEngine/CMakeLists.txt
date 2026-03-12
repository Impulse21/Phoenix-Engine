# PhxEngine Library (Main engine)

file(GLOB_RECURSE PHXENGINE_SOURCES
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

file(GLOB_RECURSE PHXENGINE_HEADERS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/PhxEngine/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/PhxEngine/*.hpp"
)

add_library(PhxEngine STATIC
    ${PHXENGINE_SOURCES}
    ${PHXENGINE_HEADERS}
)

target_include_directories(PhxEngine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(PhxEngine
    PUBLIC
        PhxCore
        PhxRhi
        PhxRenderer
        PhxAsset
        PhxResource
        PhxWorld
        glfw
        ImGui
        yaml-cpp::yaml-cpp
    PRIVATE
)

# YAML static define
target_compile_definitions(PhxEngine
    PRIVATE
        YAML_CPP_STATIC_DEFINE
)



if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/PhxEngine_pch.h")
    target_precompile_headers(PhxEngine PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/PhxEngine_pch.h"
    )
endif()


# Visual Studio organization
set_target_properties(PhxEngine PROPERTIES FOLDER "PhxLibs")

# Apply source filters for Visual Studio
if(COMMAND apply_filters)
    apply_filters(PhxEngine)
endif()