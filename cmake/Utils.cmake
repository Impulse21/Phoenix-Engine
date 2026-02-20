#==============================================================================
# Utils.cmake - Utility Functions for Phoenix Engine
#==============================================================================

#==============================================================================
# Optimize Vendor Target
#==============================================================================
# Forces vendor/third-party libraries to build with optimization even in Debug
# This improves performance without affecting your debug experience
#
# Usage: optimize_vendor_target(target_name)
#==============================================================================

function(optimize_vendor_target target_name)
    if(NOT TARGET ${target_name})
        message(WARNING "optimize_vendor_target: Target '${target_name}' does not exist")
        return()
    endif()
    
    # Get target type
    get_target_property(TARGET_TYPE ${target_name} TYPE)
    
    # Skip INTERFACE libraries (header-only)
    if(TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
        message(STATUS " [Optimization] Skipped ${target_name} (INTERFACE library)")
        return()
    endif()
    
    # ---------------------------------------------------------
    # Linux / macOS (Clang, GCC)
    # ---------------------------------------------------------
    if(NOT MSVC)
        target_compile_options(${target_name} PRIVATE
            # When in Debug mode, force high optimization
            $<$<CONFIG:Debug>:-O3>          # Maximum optimization
            $<$<CONFIG:Debug>:-g>           # Keep debug symbols
            $<$<CONFIG:Debug>:-DNDEBUG>     # Define NDEBUG (disables asserts)
        )
    
    # ---------------------------------------------------------
    # Windows (Visual Studio / MSVC)
    # ---------------------------------------------------------
    else()
        # Get current compile options to check for /RTC
        get_target_property(CURRENT_OPTIONS ${target_name} COMPILE_OPTIONS)
        
        target_compile_options(${target_name} PRIVATE
            $<$<CONFIG:Debug>:
                /O2         # Maximize speed
                /Ob2        # Aggressive inlining
                /Oi         # Enable intrinsic functions
                /Ot         # Favor fast code over small code
                /GL         # Whole program optimization
                /GS-        # Disable buffer security check (for speed)
                /DNDEBUG    # Define NDEBUG (disables asserts)
            >
        )
        
        # Disable runtime checks (incompatible with /O2)
        # This prevents D9025 warning
        target_compile_options(${target_name} PRIVATE
            $<$<CONFIG:Debug>:/RTC->  # Remove all runtime checks
        )
        
        # Enable link-time optimization for Release builds
        set_target_properties(${target_name} PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
            INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE
        )
    endif()
    
    message(STATUS " [Optimization] Applied to: ${target_name} (${TARGET_TYPE})")
endfunction()

#==============================================================================
# Suppress Warnings for Vendor Target
#==============================================================================
# Disables all warnings for a third-party library
# Use this instead of manually adding -w to every target
#
# Usage: suppress_vendor_warnings(target_name)
#==============================================================================

function(suppress_vendor_warnings target_name)
    if(NOT TARGET ${target_name})
        message(WARNING "suppress_vendor_warnings: Target '${target_name}' does not exist")
        return()
    endif()
    
    # Get target type
    get_target_property(TARGET_TYPE ${target_name} TYPE)
    
    # Skip INTERFACE libraries
    if(TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()
    
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /w)
    else()
        target_compile_options(${target_name} PRIVATE -w)
    endif()
endfunction()

#==============================================================================
# Configure Vendor Target (Combined)
#==============================================================================
# Applies both optimization and warning suppression to a vendor target
# Also sets up SYSTEM includes if the target has INTERFACE_INCLUDE_DIRECTORIES
#
# Usage: configure_vendor_target(target_name [FOLDER "Vendors/Category"])
#==============================================================================

function(configure_vendor_target target_name)
    if(NOT TARGET ${target_name})
        message(WARNING "configure_vendor_target: Target '${target_name}' does not exist")
        return()
    endif()
    
    # Parse optional arguments
    cmake_parse_arguments(
        VENDOR              # Prefix
        ""                  # Options (flags)
        "FOLDER"            # Single-value arguments
        ""                  # Multi-value arguments
        ${ARGN}
    )
    
    # Get target type
    get_target_property(TARGET_TYPE ${target_name} TYPE)
    
    # Set Visual Studio folder if provided
    if(VENDOR_FOLDER)
        set_target_properties(${target_name} PROPERTIES FOLDER "${VENDOR_FOLDER}")
    endif()
    
    # Make includes SYSTEM (suppress warnings from headers)
    get_target_property(INCLUDE_DIRS ${target_name} INTERFACE_INCLUDE_DIRECTORIES)
    if(INCLUDE_DIRS)
        set_target_properties(${target_name} PROPERTIES
            INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${INCLUDE_DIRS}"
        )
    endif()
    
    # Skip further configuration for INTERFACE libraries
    if(TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
        message(STATUS " [Vendor Config] ${target_name} (INTERFACE - headers only)")
        return()
    endif()
    
    # Apply optimizations
    optimize_vendor_target(${target_name})
    
    # Suppress warnings
    suppress_vendor_warnings(${target_name})
    
    message(STATUS " [Vendor Config] Complete: ${target_name}")
endfunction()

#==============================================================================
# Restore Build Configuration
#==============================================================================
# Restores CMAKE_BUILD_TYPE after forcing Release for dependencies
# This is called at the end of Dependencies.cmake
#
# Usage: restore_build_type(original_type)
#==============================================================================

function(restore_build_type original_type)
    if(original_type)
        set(CMAKE_BUILD_TYPE ${original_type} PARENT_SCOPE)
        message(STATUS "Restored build type to: ${original_type}")
    endif()
endfunction()
