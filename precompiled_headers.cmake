# This file is included automatically when each target is created
# It attaches rat_precompiled_headers to every target
if(TARGET rat_precompiled_headers)
    set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES rat_precompiled_headers
    )
endif()
