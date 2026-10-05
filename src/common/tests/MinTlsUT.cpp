// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <MinTls.h>
#include <mbedtls/gcm.h>
#include <mbedtls/sha256.h>
#include <mbedtls/x509_crt.h>
#include <cerrno>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

namespace
{
int64_t Deadline()
{
    struct timespec now = {};

    if (clock_gettime(CLOCK_MONOTONIC, &now))
    {
        return 0;
    }

    return static_cast<int64_t>(now.tv_sec) * 1000000000 + now.tv_nsec + 5000000000;
}

void ClearOverrides()
{
    for (const char* name : {"SSL_CERT_FILE", "SSL_CERT_DIR", "OPENSSL_CONF",
        "OPENSSL_CONF_INCLUDE", "OPENSSL_MODULES", "OPENSSL_FIPS", "OPENSSL_FORCE_FIPS_MODE"})
    {
        ASSERT_EQ(0, unsetenv(name));
    }
}

void InvalidTrust()
{
    MinTls* tls = nullptr;

    ClearOverrides();
    EXPECT_NE(0, MinTlsCreate(&tls, "/nonexistent-osconfig-test-ca.pem", Deadline(), nullptr));
    EXPECT_EQ(nullptr, tls);
    EXPECT_NE(0, MinTlsCreate(&tls, "", Deadline(), nullptr));
    EXPECT_EQ(nullptr, tls);
    EXPECT_NE(0, MinTlsCreate(&tls, "/dev/null", Deadline(), nullptr));
    EXPECT_EQ(nullptr, tls);
    EXPECT_NE(0, MinTlsCreate(&tls, "/tmp", Deadline(), nullptr));
    EXPECT_EQ(nullptr, tls);
    ASSERT_EQ(0, setenv("SSL_CERT_FILE", "/nonexistent-osconfig-test-ca.pem", 1));
    EXPECT_NE(0, MinTlsCreate(&tls, nullptr, Deadline(), nullptr));
    EXPECT_EQ(nullptr, tls);
    ASSERT_EQ(0, unsetenv("SSL_CERT_FILE"));
    ASSERT_EQ(0, setenv("SSL_CERT_DIR", "/etc/ssl/certs", 1));
    EXPECT_EQ(ENOTSUP, MinTlsCreate(&tls, nullptr, Deadline(), nullptr));
    EXPECT_EQ(nullptr, tls);
}

void ExplicitPolicy()
{
    MinTls* tls = nullptr;

    ClearOverrides();

    for (const char* name : {"OPENSSL_CONF", "OPENSSL_CONF_INCLUDE", "OPENSSL_MODULES",
        "OPENSSL_FIPS", "OPENSSL_FORCE_FIPS_MODE"})
    {
        ASSERT_EQ(0, setenv(name, "", 1));
        tls = nullptr;
        EXPECT_EQ(ENOTSUP, MinTlsCreate(&tls, nullptr, Deadline(), nullptr));
        EXPECT_EQ(nullptr, tls);
        ASSERT_EQ(0, unsetenv(name));
    }
}
}

TEST(MinTls, InvalidArgumentsAndExpiredDeadline)
{
    MinTls* tls = nullptr;
    char byte = 0;
    size_t size = 99;
    bool eof = true;

    EXPECT_EQ(EINVAL, MinTlsCreate(nullptr, nullptr, Deadline(), nullptr));
    EXPECT_EQ(ETIMEDOUT, MinTlsCreate(&tls, nullptr, 0, nullptr));
    EXPECT_EQ(nullptr, tls);
    EXPECT_EQ(EINVAL, MinTlsHandshake(nullptr, -1, nullptr, Deadline(), nullptr));
    EXPECT_EQ(EINVAL, MinTlsWrite(nullptr, nullptr, 1, Deadline(), nullptr));
    EXPECT_EQ(EINVAL, MinTlsRead(nullptr, &byte, 1, &size, &eof, Deadline(), nullptr));
    EXPECT_EQ(0U, size);
    EXPECT_FALSE(eof);
    MinTlsDestroy(nullptr, nullptr);
    MinTlsDestroy(&tls, nullptr);
}

TEST(MinTlsDeathTest, DoesNotReplaceExplicitInvalidTrustWithSystemRoots)
{
    EXPECT_EXIT(
    {
        InvalidTrust();
        _exit(::testing::Test::HasFailure() ? 1 : 0);
    },
        ::testing::ExitedWithCode(0), "");
}

TEST(MinTlsDeathTest, DoesNotBypassExplicitOpenSslPolicy)
{
    EXPECT_EXIT(
    {
        ExplicitPolicy();
        _exit(::testing::Test::HasFailure() ? 1 : 0);
    },
        ::testing::ExitedWithCode(0), "");
}

TEST(MinTlsCore, Sha256KnownAnswer)
{
    const unsigned char expected[] = {
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
        0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
        0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
    };
    unsigned char digest[32] = {};

    ASSERT_EQ(0, mbedtls_sha256(reinterpret_cast<const unsigned char*>("abc"), 3, digest, 0));
    EXPECT_EQ(0, memcmp(digest, expected, sizeof(expected)));
}

TEST(MinTlsCore, AesGcmKnownAnswerAndAuthenticationFailure)
{
    const unsigned char expected[] = {
        0x03, 0x88, 0xda, 0xce, 0x60, 0xb6, 0xa3, 0x92,
        0xf3, 0x28, 0xc2, 0xb9, 0x71, 0xb2, 0xfe, 0x78
    };
    const unsigned char expectedTag[] = {
        0xab, 0x6e, 0x47, 0xd4, 0x2c, 0xec, 0x13, 0xbd,
        0xf5, 0x3a, 0x67, 0xb2, 0x12, 0x57, 0xbd, 0xdf
    };
    unsigned char key[16] = {}, nonce[12] = {}, plaintext[16] = {};
    unsigned char ciphertext[16] = {}, tag[16] = {}, decoded[16] = {};
    mbedtls_gcm_context context = {};

    mbedtls_gcm_init(&context);
    EXPECT_EQ(0, mbedtls_gcm_setkey(&context, MBEDTLS_CIPHER_ID_AES, key, 128));
    EXPECT_EQ(0, mbedtls_gcm_crypt_and_tag(&context, MBEDTLS_GCM_ENCRYPT, sizeof(plaintext),
        nonce, sizeof(nonce), nullptr, 0, plaintext, ciphertext, sizeof(tag), tag));
    EXPECT_EQ(0, memcmp(ciphertext, expected, sizeof(expected)));
    EXPECT_EQ(0, memcmp(tag, expectedTag, sizeof(expectedTag)));
    EXPECT_EQ(0, mbedtls_gcm_auth_decrypt(&context, sizeof(ciphertext), nonce, sizeof(nonce),
        nullptr, 0, tag, sizeof(tag), ciphertext, decoded));
    EXPECT_EQ(0, memcmp(decoded, plaintext, sizeof(plaintext)));
    tag[0] ^= 1;
    EXPECT_EQ(MBEDTLS_ERR_GCM_AUTH_FAILED, mbedtls_gcm_auth_decrypt(&context,
        sizeof(ciphertext), nonce, sizeof(nonce), nullptr, 0, tag, sizeof(tag), ciphertext, decoded));
    mbedtls_gcm_free(&context);
}

TEST(MinTlsCore, CertificateProfileRejectsWeakPeerSignaturesAndKeys)
{
    EXPECT_EQ(0U, mbedtls_x509_crt_profile_default.allowed_mds & MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA1));
    EXPECT_EQ(0U, mbedtls_x509_crt_profile_default.allowed_mds & MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_MD5));
    EXPECT_GE(mbedtls_x509_crt_profile_default.rsa_min_bitlen, 2048U);
}
