# PhxRenderer Library

file(GLOB_RECURSE PHXRENDERER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

file(GLOB_RECURSE PHXRENDERER_HEADERS
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
        meshoptimizer
        stb
        bc7enc_rdo
    PRIVATE
)
