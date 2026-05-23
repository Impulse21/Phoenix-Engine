function(phx_add_app target)
    set_target_properties(${target} PROPERTIES
        WIN32_EXECUTABLE TRUE
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    target_link_libraries(${target} PRIVATE PhxEngine)
endfunction()

add_subdirectory(Cube)