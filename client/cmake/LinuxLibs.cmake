# cmake/LinuxLibs.cmake
if(UNIX)
    # Core libraries from micromamba
    find_package(fmt REQUIRED)
    find_package(CURL REQUIRED)
    find_package(ZLIB REQUIRED)
    # zstd (Conda package might not ship a CMake config)
    find_package(ZSTD QUIET)
    if(NOT ZSTD_FOUND)
        add_library(ZSTD::ZSTD STATIC IMPORTED GLOBAL)
        set_target_properties(ZSTD::ZSTD PROPERTIES
            IMPORTED_LOCATION "$ENV{CONDA_PREFIX}/lib/libzstd.a"
            INTERFACE_INCLUDE_DIRECTORIES "$ENV{CONDA_PREFIX}/include"
        )
    endif()

    # simdjson (same logic)
    find_package(simdjson QUIET)
    if(NOT simdjson_FOUND)
        add_library(simdjson STATIC IMPORTED GLOBAL)
        set_target_properties(simdjson PROPERTIES
            IMPORTED_LOCATION "$ENV{CONDA_PREFIX}/lib/libsimdjson.a"
            INTERFACE_INCLUDE_DIRECTORIES "$ENV{CONDA_PREFIX}/include"
        )
    endif()

    # tiny-process-library (manual only)
    add_library(tiny-process-library STATIC IMPORTED GLOBAL)
    set_target_properties(tiny-process-library PROPERTIES
        IMPORTED_LOCATION "$ENV{CONDA_PREFIX}/lib/libtiny-process-library.a"
        INTERFACE_INCLUDE_DIRECTORIES "$ENV{CONDA_PREFIX}/include"
    )
    add_library(TINY_PROCESS_LIBRARY::tiny-process-library ALIAS tiny-process-library)

endif()

