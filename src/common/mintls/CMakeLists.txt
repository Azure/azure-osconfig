# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(mintls STATIC
    MinTls.c
    core/library/aes.c
    core/library/asn1parse.c
    core/library/asn1write.c
    core/library/base64.c
    core/library/bignum.c
    core/library/bignum_core.c
    core/library/bignum_mod.c
    core/library/bignum_mod_raw.c
    core/library/block_cipher.c
    core/library/cipher.c
    core/library/cipher_wrap.c
    core/library/constant_time.c
    core/library/ctr_drbg.c
    core/library/ecdh.c
    core/library/ecdsa.c
    core/library/ecp.c
    core/library/ecp_curves.c
    core/library/entropy.c
    core/library/entropy_poll.c
    core/library/gcm.c
    core/library/md.c
    core/library/oid.c
    core/library/pem.c
    core/library/pk.c
    core/library/pk_ecc.c
    core/library/pk_wrap.c
    core/library/pkparse.c
    core/library/platform_util.c
    core/library/rsa.c
    core/library/rsa_alt_helpers.c
    core/library/sha1.c
    core/library/sha256.c
    core/library/sha512.c
    core/library/x509.c
    core/library/x509_crt.c
    core/library/ssl_ciphersuites.c
    core/library/ssl_client.c
    core/library/ssl_msg.c
    core/library/ssl_tls.c
    core/library/ssl_tls12_client.c)
target_include_directories(mintls PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE core/include core/library)
target_compile_definitions(mintls PRIVATE MBEDTLS_CONFIG_FILE="MinTlsConfig.h")
target_link_libraries(mintls PUBLIC logging)
set_target_properties(mintls PROPERTIES C_VISIBILITY_PRESET hidden)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mintls PRIVATE -ffunction-sections -fdata-sections
        "$<$<NOT:$<CONFIG:Debug>>:-Os>")
    target_link_options(mintls INTERFACE -Wl,--gc-sections)
endif()

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
