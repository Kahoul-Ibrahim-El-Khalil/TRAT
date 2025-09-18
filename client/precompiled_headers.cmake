if(TARGET rat_precompiled_headers)
    set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES rat_precompiled_headers
    )
endif()
