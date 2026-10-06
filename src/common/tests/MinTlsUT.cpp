// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <MinTls.h>
#include <debug.h>
#include <ecp.h>
#include <gcm.h>
#include <md.h>
#include <sha256.h>
#include <x509_crt.h>
#include <cerrno>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iterator>
#include <string>
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

TEST(MinTlsCore, RetainsOnlyTheConfiguredAuthenticatedCipherSuites)
{
    const int expected[] = {
        MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
        MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
        MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,
        MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384
    };
    const int* suites = mbedtls_ssl_list_ciphersuites();
    const mbedtls_ssl_ciphersuite_t* suite = nullptr;

    ASSERT_NE(nullptr, suites);

    for (size_t i = 0; i < ARRAY_SIZE(expected); ++i)
    {
        ASSERT_EQ(expected[i], suites[i]);
        suite = mbedtls_ssl_ciphersuite_from_id(suites[i]);
        ASSERT_NE(nullptr, suite);
        EXPECT_EQ(MBEDTLS_SSL_VERSION_TLS1_2, suite->MBEDTLS_PRIVATE(min_tls_version));
        EXPECT_EQ(MBEDTLS_SSL_VERSION_TLS1_2, suite->MBEDTLS_PRIVATE(max_tls_version));
        EXPECT_EQ(0, suite->MBEDTLS_PRIVATE(flags));
    }

    EXPECT_EQ(0, suites[ARRAY_SIZE(expected)]);
    EXPECT_EQ(nullptr, mbedtls_ssl_ciphersuite_from_id(MBEDTLS_TLS_RSA_WITH_AES_128_GCM_SHA256));
    EXPECT_EQ(nullptr, mbedtls_ssl_ciphersuite_from_id(MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA));
    EXPECT_EQ(nullptr, mbedtls_ssl_ciphersuite_from_id(MBEDTLS_TLS1_3_AES_128_GCM_SHA256));
}

TEST(MinTlsCore, RetainsTheThreeRequiredNistCurves)
{
    const mbedtls_ecp_group_id expected[] = {
        MBEDTLS_ECP_DP_SECP256R1, MBEDTLS_ECP_DP_SECP384R1, MBEDTLS_ECP_DP_SECP521R1
    };
    const mbedtls_ecp_curve_info* curves = mbedtls_ecp_curve_list();
    mbedtls_ecp_group group = {};
    size_t count = 0;
    bool matched = false;

    ASSERT_NE(nullptr, curves);

    for (; MBEDTLS_ECP_DP_NONE != curves[count].grp_id; ++count)
    {
        ASSERT_LT(count, ARRAY_SIZE(expected));
        matched = false;

        for (const auto id : expected)
        {
            matched = matched || (id == curves[count].grp_id);
        }

        EXPECT_TRUE(matched);
    }

    EXPECT_EQ(ARRAY_SIZE(expected), count);

    for (const auto id : expected)
    {
        mbedtls_ecp_group_init(&group);
        EXPECT_EQ(0, mbedtls_ecp_group_load(&group, id));
        mbedtls_ecp_group_free(&group);
    }
}

TEST(MinTlsCore, KeepsLegacyAnchorHashParsingWithoutAddingLegacyPeerAlgorithms)
{
    EXPECT_NE(nullptr, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1));
    EXPECT_NE(nullptr, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256));
    EXPECT_NE(nullptr, mbedtls_md_info_from_type(MBEDTLS_MD_SHA384));
    EXPECT_NE(nullptr, mbedtls_md_info_from_type(MBEDTLS_MD_SHA512));
    EXPECT_EQ(nullptr, mbedtls_md_info_from_type(MBEDTLS_MD_MD5));
    EXPECT_EQ(nullptr, mbedtls_md_info_from_type(MBEDTLS_MD_RIPEMD160));
    EXPECT_EQ(0U, mbedtls_x509_crt_profile_default.allowed_mds & MBEDTLS_X509_ID_FLAG(MBEDTLS_MD_SHA1));
}

class MinTlsDiagnosticsTest : public ::testing::Test
{
protected:
    char path[64] = "/tmp/osconfig-mintls-diagnostics-XXXXXX";
    OsConfigLogHandle log = nullptr;
    mbedtls_ssl_context session = {};
    mbedtls_ssl_config configuration = {};
    bool created = false;
    LoggingLevel previousLevel = LoggingLevelInformational;

    void SetUp() override
    {
        int descriptor = -1;

        previousLevel = GetLoggingLevel();
        SetLoggingLevel(LoggingLevelDebug);
        mbedtls_ssl_init(&session);
        mbedtls_ssl_config_init(&configuration);
        descriptor = mkstemp(path);
        ASSERT_GE(descriptor, 0);
        created = true;
        ASSERT_EQ(0, close(descriptor));
        log = OpenLog(path, nullptr);
        ASSERT_NE(nullptr, log);
        ASSERT_NE(nullptr, GetLogFile(log));
        ASSERT_EQ(0, mbedtls_ssl_config_defaults(&configuration, MBEDTLS_SSL_IS_CLIENT,
            MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT));
        mbedtls_ssl_conf_dbg(&configuration, MinTlsLogCallback, log);
        ASSERT_EQ(0, mbedtls_ssl_setup(&session, &configuration));
    }

    void TearDown() override
    {
        mbedtls_ssl_free(&session);
        mbedtls_ssl_config_free(&configuration);
        CloseLog(&log);
        SetLoggingLevel(previousLevel);

        if (created)
        {
            EXPECT_EQ(0, unlink(path));
        }
    }

    std::string Contents()
    {
        std::ifstream stream(path);

        return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    }
};

TEST_F(MinTlsDiagnosticsTest, RoutesCoreFailureToTheSuppliedLogAndPreservesErrno)
{
    mbedtls_ssl_context* ssl = &session;
    std::string contents;

    errno = EBUSY;
    MBEDTLS_SSL_DEBUG_RET(1, "synthetic certificate verification", MBEDTLS_ERR_X509_CERT_VERIFY_FAILED);
    EXPECT_EQ(EBUSY, errno);
    contents = Contents();
    EXPECT_NE(std::string::npos, contents.find("MinTls core:"));
    EXPECT_NE(std::string::npos, contents.find("MinTlsUT.cpp"));
    EXPECT_NE(std::string::npos, contents.find("synthetic certificate verification"));
    EXPECT_NE(std::string::npos, contents.find("core status:"));
}

TEST_F(MinTlsDiagnosticsTest, DoesNotLogReadinessSuccessCloseNotifyOrSensitiveBuffers)
{
    mbedtls_ssl_context* ssl = &session;
    int evaluated = 0;

    MBEDTLS_SSL_DEBUG_RET(1, "success", 0);
    MBEDTLS_SSL_DEBUG_RET(1, "read readiness", MBEDTLS_ERR_SSL_WANT_READ);
    MBEDTLS_SSL_DEBUG_RET(1, "write readiness", MBEDTLS_ERR_SSL_WANT_WRITE);
    MBEDTLS_SSL_DEBUG_RET(1, "authenticated close", MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY);
    MBEDTLS_SSL_DEBUG_BUF(1, "secret", (++evaluated, nullptr), 1);
    MBEDTLS_SSL_DEBUG_MPI(1, "key", (++evaluated, nullptr));
    MBEDTLS_SSL_DEBUG_CRT(1, "certificate", (++evaluated, nullptr));
    MBEDTLS_SSL_DEBUG_MSG(1, ("sensitive: %d", ++evaluated));
    EXPECT_EQ(0, evaluated);
    EXPECT_TRUE(Contents().empty());
}

TEST_F(MinTlsDiagnosticsTest, PublicFailureUsesTheCallerLog)
{
    MinTls* tls = nullptr;
    std::string contents;

    EXPECT_EQ(ETIMEDOUT, MinTlsCreate(&tls, nullptr, 0, log));
    EXPECT_EQ(nullptr, tls);
    contents = Contents();
    EXPECT_NE(std::string::npos, contents.find("MinTls: initialization failed"));
    EXPECT_NE(std::string::npos, contents.find("status:"));
}
