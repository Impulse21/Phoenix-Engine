# PhxAssetCompiler Library

file(GLOB_RECURSE PHX_RESOURCE_COMPILER_SOURCES
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

file(GLOB_RECURSE PHX_ASSET_COMPILER_HEADERS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/PhxAssetCompiler/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/PhxAssetCompiler/*.hpp"
)

add_library(PhxResourceCompiler STATIC
    ${PHX_RESOURCE_COMPILER_SOURCES}
    ${PHX_ASSET_COMPILER_HEADERS}
)

target_include_directories(PhxResourceCompiler
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(PhxResourceCompiler
    PUBLIC
        PhxCore
        PhxAsset
        PhxRenderer
        PhxResource
        PhxWorld
    PRIVATE
)

# Platform-specific links
if(PLATFORM_WINDOWS)
    target_link_libraries(PhxResourceCompiler PUBLIC
        # Windows-specific system libraries if needed
    )
elseif(PLATFORM_LINUX)
    target_link_libraries(PhxResourceCompiler PUBLIC
        pthread
        dl
    )
endif()

target_compile_definitions(PhxResourceCompiler
    PUBLIC
        $<$<CONFIG:Debug>:PHX_DEBUG>
        $<$<CONFIG:RelWithDebInfo>:PHX_RELWITHDEBINFO>
        $<$<CONFIG:Release>:PHX_RELEASE>
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/PhxResourceCompiler_pch.h")
    target_precompile_headers(PhxResourceCompiler PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/PhxResourceCompiler_pch.h"
    )
endif()


# Visual Studio organization
set_target_properties(PhxResourceCompiler PROPERTIES FOLDER "PhxLibs")

# Apply source filters for Visual Studio
if(COMMAND apply_filters)
    apply_filters(PhxResourceCompiler)
endif()