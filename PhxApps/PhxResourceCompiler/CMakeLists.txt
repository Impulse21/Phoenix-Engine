# PhxResourceCompilerApp Application

file(GLOB_RECURSE PHX_RESOURCE_COMPILER_APP_SOURCES
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "src/*.cpp"
    "src/*.c"
    "src/*.cc"
)

file(GLOB_RECURSE PHX_RESOURCE_COMPILER_APP_HEADERS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "src/*.h"
    "src/*.hpp"
)

if(PLATFORM_WINDOWS)
    # Add resource files for Windows
    file(GLOB PHXRUNTIME_RESOURCES
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "src/*.rc"
        "src/*.ico"
        "src/*.png"
    )
    list(APPEND PHX_RESOURCE_COMPILER_APP_SOURCES ${PHXRUNTIME_RESOURCES})

    add_compile_definitions(_MBCS)
endif()

add_executable(PhxResourceCompilerApp
    ${PHX_RESOURCE_COMPILER_APP_SOURCES}
    ${PHX_RESOURCE_COMPILER_APP_HEADERS}
    #${PHXRUNTIME_VENDOR_SOURCES}
)

target_include_directories(PhxResourceCompilerApp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor
        ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_link_libraries(PhxResourceCompilerApp
    PRIVATE
        PhxResourceCompiler
        bc7enc_rdo
)

# YAML static define
target_compile_definitions(PhxResourceCompilerApp
    PRIVATE
        YAML_CPP_STATIC_DEFINE
)

# Set as GUI application on Windows
if(PLATFORM_WINDOWS)
    set_target_properties(PhxResourceCompilerApp PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Set output name
set_target_properties(PhxResourceCompilerApp PROPERTIES
    OUTPUT_NAME "PhxResourceCompilerApp"
)

# Post-build: Copy Slang DLLs/SOs to output directory
if(PLATFORM_WINDOWS)
    add_custom_command(TARGET PhxResourceCompilerApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${slang_SOURCE_DIR}/bin/slang.dll"
            "${slang_SOURCE_DIR}/bin/slang-llvm.dll"
            "${slang_SOURCE_DIR}/bin/slang-rt.dll"
            $<TARGET_FILE_DIR:PhxResourceCompilerApp>
        COMMENT "Copying Slang DLLs to output directory"
    )
elseif(PLATFORM_LINUX)
    add_custom_command(TARGET PhxResourceCompilerApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${slang_SOURCE_DIR}/lib/libslang.so"
            "${slang_SOURCE_DIR}/lib/libslang-llvm.so"
            "${slang_SOURCE_DIR}/lib/libslang-rt.so"
            $<TARGET_FILE_DIR:PhxResourceCompilerApp>
        COMMENT "Copying Slang shared libraries to output directory"
    )
endif()

# Visual Studio organization
set_target_properties(PhxResourceCompilerApp PROPERTIES FOLDER "PhxApps")

# Apply source filters for Visual Studio
if(COMMAND apply_filters)
    apply_filters(PhxResourceCompilerApp)
endif()