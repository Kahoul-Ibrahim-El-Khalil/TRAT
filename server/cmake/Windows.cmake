if(WIN32)
    set(OPENSSL_DLL_DIR "X:/msys2/mingw64/bin")

    add_custom_command(TARGET server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENSSL_DLL_DIR}/libcrypto-3-x64.dll"
            $<TARGET_FILE_DIR:server>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENSSL_DLL_DIR}/libssl-3-x64.dll"
            $<TARGET_FILE_DIR:server>
        COMMENT "Copying OpenSSL DLLs into output directory")
endif()
