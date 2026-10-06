# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(mintls STATIC
    MinTls.c
    MinTlsLog.c
    aes.c
    asn1parse.c
    asn1write.c
    base64.c
    bignum.c
    bignum_core.c
    cipher.c
    cipher_wrap.c
    constant_time.c
    ctr_drbg.c
    ecdh.c
    ecdsa.c
    ecp.c
    ecp_curves.c
    entropy.c
    entropy_poll.c
    gcm.c
    md.c
    oid.c
    pem.c
    pk.c
    pk_ecc.c
    pk_wrap.c
    pkparse.c
    platform_util.c
    rsa.c
    rsa_alt_helpers.c
    sha1.c
    sha256.c
    sha512.c
    x509.c
    x509_crt.c
    ssl_ciphersuites.c
    ssl_client.c
    ssl_msg.c
    ssl_tls.c
    ssl_tls12_client.c)
target_include_directories(mintls PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mintls PUBLIC logging)
set_target_properties(mintls PROPERTIES C_VISIBILITY_PRESET hidden)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mintls PRIVATE -ffunction-sections -fdata-sections
        "$<$<NOT:$<CONFIG:Debug>>:-Os>")
    target_link_options(mintls INTERFACE -Wl,--gc-sections)
endif()
