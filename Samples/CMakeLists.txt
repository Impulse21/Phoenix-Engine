function(phx_add_app target)
    set_target_properties(${target} PROPERTIES
        WIN32_EXECUTABLE FALSE
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    target_link_libraries(${target} PRIVATE PhxEngine)

    target_include_directories(${target} PRIVATE
        "${CMAKE_SOURCE_DIR}/Engine/Include"
    )

    if (PHX_PLATFORM_WINDOWS)
        target_link_options(${target} PRIVATE /ENTRY:mainCRTStartup)
    endif()
endfunction()

add_subdirectory(Cube)