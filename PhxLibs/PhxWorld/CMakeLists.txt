# PhxWorld Library

file(GLOB_RECURSE PHXWORLD_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

file(GLOB_RECURSE PHXWORLD_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/PhxWorld/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/PhxWorld/*.hpp"
)

add_library(PhxWorld STATIC
    ${PHXWORLD_SOURCES}
    ${PHXWORLD_HEADERS}
)

target_include_directories(PhxWorld
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(PhxWorld
    PUBLIC
        PhxCore
        PhxRhi
        PhxRenderer
        PhxResource
        EnTT::EnTT
    PRIVATE
)


if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/PhxWorld_pch.h")
    target_precompile_headers(PhxWorld PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/PhxWorld_pch.h"
    )
endif()