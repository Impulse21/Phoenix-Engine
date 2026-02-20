# VisualStudioFilters.cmake
# Organize projects into folders in Visual Studio Solution Explorer

# Function to organize source files into filters matching directory structure
function(create_source_filters target_name)
    get_target_property(target_sources ${target_name} SOURCES)
    
    foreach(file_path ${target_sources})
        # Get the directory of the file relative to the target's source directory
        get_filename_component(file_dir ${file_path} DIRECTORY)
        
        # Convert path separators to backslashes for VS filters
        string(REPLACE "/" "\\" filter_path ${file_dir})
        
        # Assign to source group (filter)
        source_group("${filter_path}" FILES ${file_path})
    endforeach()
endfunction()

# Function to organize files by type (Headers, Sources, etc.)
function(create_type_filters target_name)
    get_target_property(target_sources ${target_name} SOURCES)
    
    foreach(file_path ${target_sources})
        get_filename_component(file_ext ${file_path} EXT)
        
        # Categorize by extension
        if(file_ext MATCHES "\\.(h|hpp|hxx|hh)$")
            source_group("Header Files" FILES ${file_path})
        elseif(file_ext MATCHES "\\.(cpp|cc|cxx|c)$")
            source_group("Source Files" FILES ${file_path})
        elseif(file_ext MATCHES "\\.(inl|inc)$")
            source_group("Inline Files" FILES ${file_path})
        elseif(file_ext MATCHES "\\.(natvis)$")
            source_group("Visualizers" FILES ${file_path})
        elseif(file_ext MATCHES "\\.(rc|ico|png)$")
            source_group("Resources" FILES ${file_path})
        else()
            source_group("Other Files" FILES ${file_path})
        endif()
    endforeach()
endfunction()

# Function to organize with both directory structure AND type
function(create_hybrid_filters target_name)
    get_target_property(target_sources ${target_name} SOURCES)
    
    foreach(file_path ${target_sources})
        get_filename_component(file_dir ${file_path} DIRECTORY)
        get_filename_component(file_ext ${file_path} EXT)
        
        # Determine type prefix
        if(file_ext MATCHES "\\.(h|hpp|hxx|hh)$")
            set(type_prefix "Headers")
        elseif(file_ext MATCHES "\\.(cpp|cc|cxx|c)$")
            set(type_prefix "Sources")
        else()
            set(type_prefix "Other")
        endif()
        
        # Build filter path: Type/Directory/
        if(file_dir)
            string(REPLACE "/" "\\" filter_path ${file_dir})
            set(full_filter "${type_prefix}\\${filter_path}")
        else()
            set(full_filter "${type_prefix}")
        endif()
        
        source_group("${full_filter}" FILES ${file_path})
    endforeach()
endfunction()

# Function to create custom filter structure
# Usage: create_custom_filter(MyTarget "Public API" "include/MyTarget/*.h")
function(create_custom_filter target_name filter_name pattern)
    get_target_property(target_sources ${target_name} SOURCES)
    
    foreach(file_path ${target_sources})
        if(file_path MATCHES ${pattern})
            source_group("${filter_name}" FILES ${file_path})
        endif()
    endforeach()
endfunction()

# Set default filter style for all projects
# Call this before add_subdirectory() calls
function(set_default_filter_style style)
    set(VS_FILTER_STYLE ${style} PARENT_SCOPE)
    
    if(style STREQUAL "DIRECTORY")
        message(STATUS "Visual Studio filters: Directory structure")
    elseif(style STREQUAL "TYPE")
        message(STATUS "Visual Studio filters: File types")
    elseif(style STREQUAL "HYBRID")
        message(STATUS "Visual Studio filters: Hybrid (Type + Directory)")
    else()
        message(STATUS "Visual Studio filters: Custom")
    endif()
endfunction()

# Apply filters to target based on style
function(apply_filters target_name)
    if(NOT MSVC AND NOT CMAKE_GENERATOR MATCHES "Visual Studio")
        return()
    endif()
    
    if(VS_FILTER_STYLE STREQUAL "DIRECTORY")
        create_source_filters(${target_name})
    elseif(VS_FILTER_STYLE STREQUAL "TYPE")
        create_type_filters(${target_name})
    elseif(VS_FILTER_STYLE STREQUAL "HYBRID")
        create_hybrid_filters(${target_name})
    else()
        # Default to directory structure
        create_source_filters(${target_name})
    endif()
endfunction()
