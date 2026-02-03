# PhxCore Library

file(GLOB_RECURSE PHXCORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

file(GLOB_RECURSE PHXCORE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/PhxCore/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/PhxCore/*.hpp"
)

add_library(PhxCore STATIC
    ${PHXCORE_SOURCES}
    ${PHXCORE_HEADERS}
)

target_include_directories(PhxCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(PhxCore
    PUBLIC
        spdlog::spdlog
        EnTT::EnTT
        Tracy
        hlslpp
        tlsf
    PRIVATE
)

# Platform-specific links
if(PLATFORM_WINDOWS)
    target_link_libraries(PhxCore PUBLIC
        # Windows-specific system libraries if needed
    )
elseif(PLATFORM_LINUX)
    target_link_libraries(PhxCore PUBLIC
        pthread
        dl
    )
endif()

target_compile_definitions(PhxCore
    PUBLIC
        $<$<CONFIG:Debug>:PHX_DEBUG>
        $<$<CONFIG:RelWithDebInfo>:PHX_RELWITHDEBINFO>
        $<$<CONFIG:Release>:PHX_RELEASE>
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/PhxCore_pch.h")
    target_precompile_headers(PhxCore PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/PhxCore_pch.h"
    )
endif()