# PhxRuntime Application

file(GLOB_RECURSE PHXRUNTIME_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
)

file(GLOB_RECURSE PHXRUNTIME_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

# Include vendor sources if they exist in the app directory
file(GLOB_RECURSE PHXRUNTIME_VENDOR_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/tinyobj/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/tinyobj/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/fast_obj/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/fast_obj/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/meshoptimizer/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/meshoptimizer/*.h"
)

if(PLATFORM_WINDOWS)
    # Add resource files for Windows
    file(GLOB PHXRUNTIME_RESOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.rc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ico"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.png"
    )
    list(APPEND PHXRUNTIME_SOURCES ${PHXRUNTIME_RESOURCES})
endif()

add_executable(PhxRuntime
    ${PHXRUNTIME_SOURCES}
    ${PHXRUNTIME_HEADERS}
    ${PHXRUNTIME_VENDOR_SOURCES}
)

target_include_directories(PhxRuntime
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

target_link_libraries(PhxRuntime
    PRIVATE
        PhxCore
        PhxRhi
        PhxRenderer
        PhxResource
        PhxWorld
        PhxReflection
        PhxEngine
        ImGui
        Tracy
        bc7enc_rdo
)

# YAML static define
target_compile_definitions(PhxRuntime
    PRIVATE
        YAML_CPP_STATIC_DEFINE
)

# Set as GUI application on Windows
if(PLATFORM_WINDOWS)
    set_target_properties(PhxRuntime PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Set output name
set_target_properties(PhxRuntime PROPERTIES
    OUTPUT_NAME "PhxRuntime"
)

# Post-build: Copy Slang DLLs/SOs to output directory
if(PLATFORM_WINDOWS)
    add_custom_command(TARGET PhxRuntime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${slang_SOURCE_DIR}/bin/slang.dll"
            "${slang_SOURCE_DIR}/bin/slang-llvm.dll"
            "${slang_SOURCE_DIR}/bin/slang-rt.dll"
            $<TARGET_FILE_DIR:PhxRuntime>
        COMMENT "Copying Slang DLLs to output directory"
    )
elseif(PLATFORM_LINUX)
    add_custom_command(TARGET PhxRuntime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${slang_SOURCE_DIR}/lib/libslang.so"
            "${slang_SOURCE_DIR}/lib/libslang-llvm.so"
            "${slang_SOURCE_DIR}/lib/libslang-rt.so"
            $<TARGET_FILE_DIR:PhxRuntime>
        COMMENT "Copying Slang shared libraries to output directory"
    )
endif()

# Visual Studio organization
set_target_properties(PhxRuntime PROPERTIES FOLDER "PhxApps")

# Apply source filters for Visual Studio
if(COMMAND apply_filters)
    apply_filters(PhxRuntime)
endif()