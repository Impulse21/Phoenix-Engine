# PhxRuntime Application

file(GLOB_RECURSE PHXRUNTIME_SOURCES
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "src/*.cpp"
    "src/*.c"
    "src/*.cc"
)

file(GLOB_RECURSE PHXRUNTIME_HEADERS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "src/*.h"
    "src/*.hpp"
)


# Include vendor sources if they exist in the app directory
#[[file(GLOB_RECURSE PHXRUNTIME_VENDOR_SOURCES
    "vendor/tinyobj/*.cc"
    "vendor/tinyobj/*.h"
    "vendor/fast_obj/*.c"
    "vendor/fast_obj/*.h"
    "vendor/meshoptimizer/*.cpp"
    "vendor/meshoptimizer/*.h"
)
]]

if(PLATFORM_WINDOWS)
    # Add resource files for Windows
    file(GLOB PHXRUNTIME_RESOURCES
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "src/*.rc"
        "src/*.ico"
        "src/*.png"
    )
    list(APPEND PHXRUNTIME_SOURCES ${PHXRUNTIME_RESOURCES})
endif()

add_executable(PhxRuntime
    ${PHXRUNTIME_SOURCES}
    ${PHXRUNTIME_HEADERS}
    #${PHXRUNTIME_VENDOR_SOURCES}
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
        TracyClient
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