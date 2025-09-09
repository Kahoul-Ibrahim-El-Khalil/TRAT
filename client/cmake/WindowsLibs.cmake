if(WIN32)
	set(STATIC_PATH "${CMAKE_SOURCE_DIR}/../../Static")
	set(STATIC_LIB "${STATIC_PATH}/lib")
	set(STATIC_INCLUDE "${STATIC_PATH}/include")

	message(STATUS "STATIC_LIB = ${STATIC_LIB}")
	message(STATUS "STATIC_INCLUDE = ${STATIC_INCLUDE}")
	include_directories(${STATIC_INCLUDE})
    link_directories(${STATIC_LIB})

    add_library(fmt STATIC IMPORTED GLOBAL)
    set_target_properties(fmt PROPERTIES
        IMPORTED_LOCATION "${STATIC_LIB}/libfmt.a"
        INTERFACE_INCLUDE_DIRECTORIES "${STATIC_INCLUDE}"
    )

    add_library(zlib STATIC IMPORTED GLOBAL)
    set_target_properties(zlib PROPERTIES
        IMPORTED_LOCATION "${STATIC_LIB}/libz.a"
        INTERFACE_INCLUDE_DIRECTORIES "${STATIC_INCLUDE}"
    )

    add_library(simdjson STATIC IMPORTED GLOBAL)
    set_target_properties(simdjson PROPERTIES
        IMPORTED_LOCATION "${STATIC_LIB}/libsimdjson.a"
        INTERFACE_INCLUDE_DIRECTORIES "${STATIC_INCLUDE}"
    )

    add_library(tiny-process-library STATIC IMPORTED GLOBAL)
    set_target_properties(tiny-process-library PROPERTIES
        IMPORTED_LOCATION "${STATIC_LIB}/libtiny-process-library.a"
        INTERFACE_INCLUDE_DIRECTORIES "${STATIC_INCLUDE}"
    )

    add_library(curl STATIC IMPORTED GLOBAL)
    set_target_properties(curl PROPERTIES
        IMPORTED_LOCATION "${STATIC_LIB}/libcurl.a"
        INTERFACE_INCLUDE_DIRECTORIES "${STATIC_INCLUDE}"
        INTERFACE_COMPILE_DEFINITIONS "CURL_STATICLIB"
        INTERFACE_LINK_LIBRARIES "zlib;ws2_32;bcrypt;crypt32;wldap32;normaliz"
    )

    add_library(CURL::libcurl ALIAS curl)
    add_library(FMT::fmt ALIAS fmt)
    add_library(ZLIB::zlib ALIAS zlib)
    add_library(SIMDJSON::simdjson ALIAS simdjson)
    add_library(TINY_PROCESS_LIBRARY::tiny-process-library ALIAS tiny-process-library)
endif()

