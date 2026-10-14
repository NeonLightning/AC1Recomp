include_guard(GLOBAL)

# Host utility only: both guest execution profiles retain exactly the same
# file identity. No OpenSSL installation is required for a portable build.
function(psxrecomp_sha256_file target)
    target_sources(${target} PRIVATE
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../runtime/src/psx_sha256_file.cpp")
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../runtime/include")
    if(WIN32)
        target_link_libraries(${target} PRIVATE bcrypt)
    else()
        find_package(OpenSSL QUIET COMPONENTS Crypto)
        # A sysroot toolchain may not see the host's /usr/include.
        include(CheckIncludeFileCXX)
        set(CMAKE_REQUIRED_INCLUDES ${OPENSSL_INCLUDE_DIR})
        check_include_file_cxx(openssl/evp.h PSX_HAVE_OPENSSL_EVP_H)
        unset(CMAKE_REQUIRED_INCLUDES)
        if(TARGET OpenSSL::Crypto AND PSX_HAVE_OPENSSL_EVP_H)
            target_compile_definitions(${target} PRIVATE PSX_SHA256_FILE_OPENSSL=1)
            target_link_libraries(${target} PRIVATE OpenSSL::Crypto)
        endif()
    endif()
endfunction()
