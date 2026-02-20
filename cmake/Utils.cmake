
#==============================================================================
# Utils Build Configuration
#==============================================================================
function(optimize_vendor_target target_name)
    if(TARGET ${target_name})
        # ---------------------------------------------------------
        # Linux / macOS (Clang, GCC, Ninja)
        # ---------------------------------------------------------
        if(NOT MSVC)
            target_compile_options(${target_name} PRIVATE
                # When in Debug mode, force high optimization
                $<$<CONFIG:Debug>:-O3>
                $<$<CONFIG:Debug>:-g> 
            )

        # ---------------------------------------------------------
        # Windows (Visual Studio / MSVC)
        # ---------------------------------------------------------
        else()
            target_compile_options(${target_name} PRIVATE
                $<$<CONFIG:Debug>:
                    /O2   # Maximize speed
                    /Ob2  # Aggressive inlining
                    /Oi   # Intrinsic functions
                    /Ot   # Favor fast code
                    # Note: We do NOT change /MDd (Debug Runtime). 
                    # We keep the debug runtime so it links with your app!
                >
            )
            
            # (Optional) Disable Runtime Checks (/RTC) for these targets
            # because they are incompatible with optimization (/O2).
            # This suppresses warning D9025.
            string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        endif()
        
        message(STATUS " [Optimization Override] Applied to: ${target_name}")
    endif()
endfunction()