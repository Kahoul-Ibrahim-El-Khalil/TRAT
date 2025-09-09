#cmake/LinuxLibs.cmake
if(UNIX)
    # Core libraries
    find_package(fmt REQUIRED)
    find_package(CURL REQUIRED)

	add_library(simdjson STATIC IMPORTED GLOBAL)
    set_target_properties(simdjson PROPERTIES
        IMPORTED_LOCATION "/usr/local/lib/libsimdjson.a"
        INTERFACE_INCLUDE_DIRECTORIES "/usr/local/include"
    )

    add_library(tiny-process-library STATIC IMPORTED GLOBAL)
    set_target_properties(tiny-process-library PROPERTIES
        IMPORTED_LOCATION "/usr/local/lib/libtiny-process-library.a"
        INTERFACE_INCLUDE_DIRECTORIES "/usr/local/include"
    )
    add_library(SIMDJSON::simdjson ALIAS simdjson)
    add_library(TINY_PROCESS_LIBRARY::tiny-process-library ALIAS tiny-process-library)

    # Optional logging library only if DEBUG=ON
    if(DEBUG)
        find_package(spdlog REQUIRED)
    endif()
endif()

