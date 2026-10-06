// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <MinTls.h>
#include <tls.h>
#include <cerrno>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iterator>
#include <string>
#include <unistd.h>

extern "C" int ssl_calc_finished_tls_generic(mbedtls_ssl_context* ssl, void* context,
    unsigned char* padding, size_t hashSize, unsigned char* output, int from,
    MinTlsDiagnostics* diagnostics);
extern "C" int ssl_parse_signature_algorithm(mbedtls_ssl_context* ssl, uint16_t algorithm,
    mbedtls_md_type_t* digest, mbedtls_pk_type_t* key, MinTlsDiagnostics* diagnostics);
extern "C" int ssl_parse_server_hello(mbedtls_ssl_context* ssl, MinTlsDiagnostics* diagnostics);

namespace
{
int UnexpectedDiagnosticsRandom(void*, unsigned char*, size_t, MinTlsDiagnostics*)
{
    ADD_FAILURE() << "Logging-only fixture must not request random bytes";
    return MBEDTLS_ERR_SSL_NO_RNG;
}

int FailingDiagnosticsEntropy(void* expected, unsigned char*, size_t, size_t* size,
    MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    EXPECT_EQ(expected, diagnostics);
    *size = 0;

    MINTLS_RETURN_ERROR(MBEDTLS_ERR_ENTROPY_SOURCE_FAILED);
}

int FailingDiagnosticsPrf(const unsigned char*, size_t, const char*, const unsigned char*,
    size_t, unsigned char*, size_t, MinTlsDiagnostics* diagnostics)
{
    MINTLS_BEGIN_DIAGNOSTIC();

    MINTLS_RETURN_ERROR(MBEDTLS_ERR_MD_ALLOC_FAILED);
}

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
    MinTlsDiagnostics diagnosticContext = {nullptr, nullptr};
    MinTlsDiagnostics* diagnostics = &diagnosticContext;

    const unsigned char expected[] = {
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
        0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
        0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
    };
    unsigned char digest[32] = {};

    ASSERT_EQ(0, mbedtls_sha256(reinterpret_cast<const unsigned char*>("abc"), 3, digest, 0, diagnostics));
    EXPECT_EQ(0, memcmp(digest, expected, sizeof(expected)));
}

TEST(MinTlsCore, AesGcmKnownAnswerAndAuthenticationFailure)
{
    MinTlsDiagnostics diagnosticContext = {nullptr, nullptr};
    MinTlsDiagnostics* diagnostics = &diagnosticContext;

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
    EXPECT_EQ(0, mbedtls_gcm_setkey(&context, MBEDTLS_CIPHER_ID_AES, key, 128, diagnostics));
    EXPECT_EQ(0, mbedtls_gcm_crypt_and_tag(&context, MBEDTLS_GCM_ENCRYPT, sizeof(plaintext),
        nonce, sizeof(nonce), nullptr, 0, plaintext, ciphertext, sizeof(tag), tag, diagnostics));
    EXPECT_EQ(0, memcmp(ciphertext, expected, sizeof(expected)));
    EXPECT_EQ(0, memcmp(tag, expectedTag, sizeof(expectedTag)));
    EXPECT_EQ(0, mbedtls_gcm_auth_decrypt(&context, sizeof(ciphertext), nonce, sizeof(nonce),
        nullptr, 0, tag, sizeof(tag), ciphertext, decoded, diagnostics));
    EXPECT_EQ(0, memcmp(decoded, plaintext, sizeof(plaintext)));
    tag[0] ^= 1;
    EXPECT_EQ(MBEDTLS_ERR_GCM_AUTH_FAILED, mbedtls_gcm_auth_decrypt(&context,
        sizeof(ciphertext), nonce, sizeof(nonce), nullptr, 0, tag, sizeof(tag), ciphertext, decoded, diagnostics));
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
    MinTlsDiagnostics diagnosticContext = {nullptr, nullptr};
    MinTlsDiagnostics* diagnostics = &diagnosticContext;

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
        EXPECT_EQ(0, mbedtls_ecp_group_load(&group, id, diagnostics));
        EXPECT_EQ(0, mbedtls_ecp_check_pubkey(&group, &group.G, diagnostics));
        EXPECT_EQ(nullptr, diagnosticContext.frame);
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
    MinTlsDiagnostics diagnosticContext = {nullptr, nullptr};
    MinTlsDiagnostics* diagnostics = &diagnosticContext;
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
        diagnosticContext.log = log;
        ASSERT_EQ(0, mbedtls_ssl_config_defaults(&configuration, MBEDTLS_SSL_IS_CLIENT,
            MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT));
        mbedtls_ssl_conf_rng(&configuration, UnexpectedDiagnosticsRandom, nullptr);
        ASSERT_EQ(0, mbedtls_ssl_setup(&session, &configuration, diagnostics));
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
    int evaluated = 0;

    for (const int result : {0, 1, MBEDTLS_ERR_SSL_WANT_READ,
        MBEDTLS_ERR_SSL_WANT_WRITE, MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY})
    {
        MinTlsDiagnosticFrame frame = {};

        MinTlsBeginDiagnostic(diagnostics, &frame);
        EXPECT_EQ(result, MinTlsEndDiagnostic(diagnostics, &frame,
            "normal continuation", __FILE__, __LINE__, result, false));
        EXPECT_EQ(nullptr, diagnosticContext.frame);
    }

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

TEST_F(MinTlsDiagnosticsTest, ReportsOriginThroughNestedAsn1Calls)
{
    unsigned char encoded[] = {MBEDTLS_ASN1_BOOLEAN, 0x82, 0x01};
    unsigned char* cursor = encoded;
    int value = 0;
    std::string contents;

    errno = EBUSY;
    EXPECT_EQ(MBEDTLS_ERR_ASN1_OUT_OF_DATA,
        mbedtls_asn1_get_bool(&cursor, encoded + sizeof(encoded), &value, diagnostics));
    EXPECT_EQ(EBUSY, errno);
    EXPECT_EQ(nullptr, diagnosticContext.frame);
    contents = Contents();
    EXPECT_NE(std::string::npos, contents.find("mbedtls_asn1_get_len failed"));
    EXPECT_EQ(std::string::npos, contents.find("mbedtls_asn1_get_bool failed"));
}

TEST_F(MinTlsDiagnosticsTest, DiscardsRecoveredProbeAndSuccessfulLengthResults)
{
    MinTlsDiagnosticFrame outer = {};
    unsigned char encoded[] = {MBEDTLS_ASN1_BOOLEAN, 0x01, 0xff};
    unsigned char* cursor = encoded;
    size_t length = 0;
    int value = 0;

    MinTlsBeginDiagnostic(diagnostics, &outer);
    EXPECT_EQ(MBEDTLS_ERR_ASN1_UNEXPECTED_TAG,
        mbedtls_asn1_get_tag(&cursor, encoded + sizeof(encoded), &length, MBEDTLS_ASN1_INTEGER, diagnostics));
    EXPECT_EQ(0, mbedtls_asn1_get_bool(&cursor, encoded + sizeof(encoded), &value, diagnostics));
    EXPECT_EQ(1, value);
    EXPECT_EQ(0, MinTlsEndDiagnostic(diagnostics, &outer, "successful alternative", __FILE__, __LINE__, 0, false));
    MinTlsDiagnosticFrame lengthResult = {};
    MinTlsBeginDiagnostic(diagnostics, &lengthResult);
    EXPECT_EQ(16, MinTlsEndDiagnostic(diagnostics, &lengthResult,
        "successful byte count", __FILE__, __LINE__, 16, false));
    EXPECT_EQ(nullptr, diagnosticContext.frame);
    EXPECT_TRUE(Contents().empty());
}

TEST_F(MinTlsDiagnosticsTest, DirectFailureDoesNotReuseARecoveredProbe)
{
    MinTlsDiagnosticFrame outer = {};
    unsigned char encoded[] = {MBEDTLS_ASN1_BOOLEAN};
    unsigned char* cursor = encoded;
    size_t length = 0;
    std::string contents;

    MinTlsBeginDiagnostic(diagnostics, &outer);
    EXPECT_EQ(MBEDTLS_ERR_ASN1_UNEXPECTED_TAG,
        mbedtls_asn1_get_tag(&cursor, encoded + sizeof(encoded), &length, MBEDTLS_ASN1_INTEGER, diagnostics));
    EXPECT_EQ(MBEDTLS_ERR_ASN1_UNEXPECTED_TAG, MinTlsEndDiagnostic(diagnostics, &outer,
        "new direct failure", __FILE__, __LINE__, MBEDTLS_ERR_ASN1_UNEXPECTED_TAG, true));
    contents = Contents();
    EXPECT_NE(std::string::npos, contents.find("new direct failure failed"));
    EXPECT_EQ(std::string::npos, contents.find("mbedtls_asn1_get_tag failed"));
    EXPECT_EQ(nullptr, diagnosticContext.frame);
}

TEST_F(MinTlsDiagnosticsTest, CombinedErrorRetainsLowLevelOrigin)
{
    MinTlsDiagnosticFrame outer = {};
    unsigned char encoded[] = {0x82, 0x01};
    unsigned char* cursor = encoded;
    size_t length = 0;
    int result = 0;
    std::string contents;

    MinTlsBeginDiagnostic(diagnostics, &outer);
    result = mbedtls_asn1_get_len(&cursor, encoded + sizeof(encoded), &length, diagnostics);
    EXPECT_EQ(MBEDTLS_ERR_ASN1_OUT_OF_DATA, result);
    result = MinTlsCombineDiagnostic(diagnostics, MBEDTLS_ERR_X509_INVALID_FORMAT, result);
    EXPECT_EQ(result, MinTlsEndDiagnostic(diagnostics, &outer, "certificate parser",
        __FILE__, __LINE__, result, false));
    contents = Contents();
    EXPECT_NE(std::string::npos, contents.find("mbedtls_asn1_get_len failed"));
    EXPECT_EQ(std::string::npos, contents.find("certificate parser failed"));
    EXPECT_EQ(nullptr, diagnosticContext.frame);
}

TEST_F(MinTlsDiagnosticsTest, RandomSourceCallbackReceivesContextAndReportsOriginOnce)
{
    mbedtls_entropy_context entropy = {};
    unsigned char output[16] = {};
    std::string contents;
    size_t first = 0;

    mbedtls_entropy_init(&entropy, diagnostics);
    entropy.MBEDTLS_PRIVATE(source_count) = 0;
    EXPECT_EQ(0, mbedtls_entropy_add_source(&entropy, FailingDiagnosticsEntropy,
        diagnostics, 1, MBEDTLS_ENTROPY_SOURCE_STRONG, diagnostics));
    EXPECT_EQ(MBEDTLS_ERR_ENTROPY_SOURCE_FAILED,
        mbedtls_entropy_func(&entropy, output, sizeof(output), diagnostics));
    mbedtls_entropy_free(&entropy);
    EXPECT_EQ(nullptr, diagnosticContext.frame);
    contents = Contents();
    first = contents.find("FailingDiagnosticsEntropy failed");
    ASSERT_NE(std::string::npos, first);
    EXPECT_EQ(std::string::npos, contents.find("MinTls core:", first));
}

TEST_F(MinTlsDiagnosticsTest, CryptoInputFailureLogsNoKeyBytes)
{
    mbedtls_aes_context aes = {};
    const unsigned char key[] = "sensitive-test-key-never-log";
    std::string contents;

    mbedtls_aes_init(&aes);
    EXPECT_EQ(MBEDTLS_ERR_AES_INVALID_KEY_LENGTH,
        mbedtls_aes_setkey_enc(&aes, key, 7, diagnostics));
    mbedtls_aes_free(&aes);
    EXPECT_EQ(nullptr, diagnosticContext.frame);
    contents = Contents();
    EXPECT_NE(std::string::npos, contents.find("mbedtls_aes_setkey_enc failed"));
    EXPECT_EQ(std::string::npos, contents.find("sensitive-test-key-never-log"));
}

TEST_F(MinTlsDiagnosticsTest, PemProbeDoesNotReplaceTheFirstCertificateFailure)
{
    const unsigned char pem[] =
        "-----BEGIN CERTIFICATE-----\nAA==\n-----END CERTIFICATE-----\n"
        "trailing text without another PEM header\n";
    mbedtls_x509_crt chain = {};
    std::string contents;

    mbedtls_x509_crt_init(&chain);
    EXPECT_EQ(MBEDTLS_ERR_X509_INVALID_FORMAT,
        mbedtls_x509_crt_parse(&chain, pem, sizeof(pem), diagnostics));
    mbedtls_x509_crt_free(&chain);
    EXPECT_EQ(nullptr, diagnosticContext.frame);
    contents = Contents();
    EXPECT_NE(std::string::npos, contents.find("mbedtls_asn1_get_tag failed")) << contents;
    EXPECT_NE(std::string::npos,
        contents.find("originating status: " + std::to_string(MBEDTLS_ERR_ASN1_UNEXPECTED_TAG))) << contents;
    EXPECT_EQ(std::string::npos, contents.find("mbedtls_pem_read_buffer failed"));
}

TEST_F(MinTlsDiagnosticsTest, DrbgErrorTranslationRetainsTheEntropySource)
{
    mbedtls_entropy_context entropy = {};
    mbedtls_ctr_drbg_context random = {};
    std::string contents;

    mbedtls_entropy_init(&entropy, diagnostics);
    mbedtls_ctr_drbg_init(&random);
    entropy.MBEDTLS_PRIVATE(source_count) = 0;
    EXPECT_EQ(0, mbedtls_entropy_add_source(&entropy, FailingDiagnosticsEntropy,
        diagnostics, 1, MBEDTLS_ENTROPY_SOURCE_STRONG, diagnostics));
    EXPECT_EQ(MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED,
        mbedtls_ctr_drbg_seed(&random, mbedtls_entropy_func, &entropy, nullptr, 0, diagnostics));
    mbedtls_ctr_drbg_free(&random);
    mbedtls_entropy_free(&entropy);
    EXPECT_EQ(nullptr, diagnosticContext.frame);
    contents = Contents();
    EXPECT_NE(std::string::npos, contents.find("FailingDiagnosticsEntropy failed"));
    EXPECT_EQ(std::string::npos, contents.find("mbedtls_ctr_drbg_reseed_internal failed"));
}

TEST_F(MinTlsDiagnosticsTest, FinishedMessageDoesNotHidePrfFailure)
{
    mbedtls_md_context_t hash = {};
    unsigned char padding[32] = {};
    unsigned char expected[32] = {};
    unsigned char output[12] = {};
    std::string contents;

    ASSERT_NE(nullptr, session.MBEDTLS_PRIVATE(handshake));
    ASSERT_NE(nullptr, session.MBEDTLS_PRIVATE(session_negotiate));
    session.MBEDTLS_PRIVATE(handshake)->tls_prf = FailingDiagnosticsPrf;
    mbedtls_md_init(&hash);
    ASSERT_EQ(0, mbedtls_md_setup(&hash, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0, diagnostics));
    EXPECT_EQ(0, mbedtls_md_starts(&hash, diagnostics));
    EXPECT_EQ(MBEDTLS_ERR_MD_ALLOC_FAILED, ssl_calc_finished_tls_generic(&session,
        &hash, padding, sizeof(padding), output, MBEDTLS_SSL_IS_CLIENT, diagnostics));
    mbedtls_md_free(&hash);
    EXPECT_EQ(0, memcmp(padding, expected, sizeof(padding)));
    EXPECT_EQ(nullptr, diagnosticContext.frame);
    contents = Contents();
    EXPECT_NE(std::string::npos, contents.find("FailingDiagnosticsPrf failed"));
}

TEST_F(MinTlsDiagnosticsTest, KeyPairCheckDoesNotHideGroupCopyFailure)
{
    mbedtls_ecp_keypair publicKey = {};
    mbedtls_ecp_keypair privateKey = {};
    std::string contents;

    mbedtls_ecp_keypair_init(&publicKey);
    mbedtls_ecp_keypair_init(&privateKey);
    publicKey.MBEDTLS_PRIVATE(grp).id = MBEDTLS_ECP_DP_SECP192R1;
    privateKey.MBEDTLS_PRIVATE(grp).id = MBEDTLS_ECP_DP_SECP192R1;
    EXPECT_EQ(MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE, mbedtls_ecp_check_pub_priv(&publicKey,
        &privateKey, UnexpectedDiagnosticsRandom, nullptr, diagnostics));
    mbedtls_ecp_keypair_free(&privateKey);
    mbedtls_ecp_keypair_free(&publicKey);
    EXPECT_EQ(nullptr, diagnosticContext.frame);
    contents = Contents();
    EXPECT_NE(std::string::npos, contents.find("mbedtls_ecp_group_load failed"));
}

TEST_F(MinTlsDiagnosticsTest, SecondaryAlertFailureDoesNotReplaceThePrimaryFailure)
{
    MinTlsDiagnosticFrame outer = {};
    std::string contents;

    MinTlsBeginDiagnostic(diagnostics, &outer);
    MinTlsRecordDiagnostic(diagnostics, "primary verification", __FILE__, __LINE__, MBEDTLS_ERR_X509_CERT_VERIFY_FAILED);
    EXPECT_EQ(MBEDTLS_ERR_SSL_BAD_INPUT_DATA, mbedtls_ssl_send_alert_message(nullptr,
        MBEDTLS_SSL_ALERT_LEVEL_FATAL, MBEDTLS_SSL_ALERT_MSG_BAD_CERT, diagnostics));
    EXPECT_EQ(&outer, diagnosticContext.frame);
    EXPECT_EQ(MBEDTLS_ERR_X509_CERT_VERIFY_FAILED, MinTlsEndDiagnostic(diagnostics, &outer,
        "outer operation", __FILE__, __LINE__, MBEDTLS_ERR_X509_CERT_VERIFY_FAILED, false));
    EXPECT_EQ(nullptr, diagnosticContext.frame);
    contents = Contents();
    EXPECT_NE(std::string::npos, contents.find("mbedtls_ssl_send_alert_message failed"));
    EXPECT_NE(std::string::npos, contents.find("primary verification failed"));
}

TEST_F(MinTlsDiagnosticsTest, PositiveAlertCodeIsAFailureNotAByteCount)
{
    mbedtls_md_type_t digest = MBEDTLS_MD_NONE;
    mbedtls_pk_type_t key = MBEDTLS_PK_NONE;
    std::string contents;

    EXPECT_EQ(MBEDTLS_SSL_ALERT_MSG_ILLEGAL_PARAMETER,
        ssl_parse_signature_algorithm(&session, 0xffff, &digest, &key, diagnostics));
    EXPECT_EQ(nullptr, diagnosticContext.frame);
    contents = Contents();
    EXPECT_NE(std::string::npos, contents.find("mbedtls_ssl_get_pk_type_and_md_alg_from_sig_alg failed"));
    EXPECT_NE(std::string::npos, contents.find("core status: 47"));
    EXPECT_NE(std::string::npos, contents.find("peer selected unsupported signature algorithm 0xffff"));
}

TEST_F(MinTlsDiagnosticsTest, UnsupportedCurveReportsProfileFailure)
{
    mbedtls_ecp_group group = {};
    std::string contents;

    mbedtls_ecp_group_init(&group);
    EXPECT_EQ(MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE,
        mbedtls_ecp_group_load(&group, MBEDTLS_ECP_DP_SECP192R1, diagnostics));
    mbedtls_ecp_group_free(&group);
    EXPECT_EQ(nullptr, diagnosticContext.frame);
    contents = Contents();
    EXPECT_NE(std::string::npos, contents.find("mbedtls_ecp_group_load failed")) << contents;
    EXPECT_NE(std::string::npos, contents.find("unsupported elliptic-curve operation or group")) << contents;
}

TEST_F(MinTlsDiagnosticsTest, CombinedUnsupportedSignatureReportsReason)
{
    const int result = MBEDTLS_ERR_X509_UNKNOWN_SIG_ALG + MBEDTLS_ERR_OID_NOT_FOUND;
    std::string contents;

    errno = EBUSY;
    MinTlsRecordDiagnostic(diagnostics, "certificate signature", __FILE__, __LINE__, result);
    EXPECT_EQ(EBUSY, errno);
    contents = Contents();
    EXPECT_NE(std::string::npos, contents.find("unsupported key or certificate signature algorithm")) << contents;
}

TEST_F(MinTlsDiagnosticsTest, PeerFatalAlertReportsDescription)
{
    std::string contents;

    session.MBEDTLS_PRIVATE(in_msgtype) = MBEDTLS_SSL_MSG_ALERT;
    session.MBEDTLS_PRIVATE(in_msglen) = 2;
    session.MBEDTLS_PRIVATE(in_msg)[0] = MBEDTLS_SSL_ALERT_LEVEL_FATAL;
    session.MBEDTLS_PRIVATE(in_msg)[1] = MBEDTLS_SSL_ALERT_MSG_HANDSHAKE_FAILURE;
    EXPECT_EQ(MBEDTLS_ERR_SSL_FATAL_ALERT_MESSAGE,
        mbedtls_ssl_handle_message_type(&session, diagnostics));
    EXPECT_EQ(nullptr, diagnosticContext.frame);
    contents = Contents();
    EXPECT_NE(std::string::npos, contents.find("peer fatal TLS alert, description: 40")) << contents;
    EXPECT_NE(std::string::npos, contents.find("peer rejected the TLS operation")) << contents;
}

TEST_F(MinTlsDiagnosticsTest, PeerCloseNotifyRemainsQuiet)
{
    session.MBEDTLS_PRIVATE(in_msgtype) = MBEDTLS_SSL_MSG_ALERT;
    session.MBEDTLS_PRIVATE(in_msglen) = 2;
    session.MBEDTLS_PRIVATE(in_msg)[0] = MBEDTLS_SSL_ALERT_LEVEL_WARNING;
    session.MBEDTLS_PRIVATE(in_msg)[1] = MBEDTLS_SSL_ALERT_MSG_CLOSE_NOTIFY;
    EXPECT_EQ(MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY,
        mbedtls_ssl_handle_message_type(&session, diagnostics));
    EXPECT_EQ(nullptr, diagnosticContext.frame);
    EXPECT_TRUE(Contents().empty());
}

TEST_F(MinTlsDiagnosticsTest, UnsupportedPeerVersionReportsProfile)
{
    std::string contents;

    session.MBEDTLS_PRIVATE(keep_current_message) = 1;
    session.MBEDTLS_PRIVATE(in_msgtype) = MBEDTLS_SSL_MSG_HANDSHAKE;
    session.MBEDTLS_PRIVATE(in_hslen) = 42;
    memset(session.MBEDTLS_PRIVATE(in_msg), 0, 42);
    session.MBEDTLS_PRIVATE(in_msg)[0] = MBEDTLS_SSL_HS_SERVER_HELLO;
    session.MBEDTLS_PRIVATE(in_msg)[4] = 3;
    session.MBEDTLS_PRIVATE(in_msg)[5] = 2;
    EXPECT_EQ(MBEDTLS_ERR_SSL_BAD_PROTOCOL_VERSION, ssl_parse_server_hello(&session, diagnostics));
    EXPECT_EQ(nullptr, diagnosticContext.frame);
    contents = Contents();
    EXPECT_NE(std::string::npos, contents.find("peer TLS version 0x0302 is outside the TLS 1.2 profile")) << contents;
    EXPECT_NE(std::string::npos, contents.find("ssl_parse_server_hello failed")) << contents;
}

TEST_F(MinTlsDiagnosticsTest, UnsupportedPeerCipherReportsSelection)
{
    std::string contents;

    session.MBEDTLS_PRIVATE(keep_current_message) = 1;
    session.MBEDTLS_PRIVATE(in_msgtype) = MBEDTLS_SSL_MSG_HANDSHAKE;
    session.MBEDTLS_PRIVATE(in_hslen) = 42;
    memset(session.MBEDTLS_PRIVATE(in_msg), 0, 42);
    session.MBEDTLS_PRIVATE(in_msg)[0] = MBEDTLS_SSL_HS_SERVER_HELLO;
    session.MBEDTLS_PRIVATE(in_msg)[4] = 3;
    session.MBEDTLS_PRIVATE(in_msg)[5] = 3;
    session.MBEDTLS_PRIVATE(in_msg)[39] = 0x13;
    session.MBEDTLS_PRIVATE(in_msg)[40] = 0x01;
    EXPECT_EQ(MBEDTLS_ERR_SSL_BAD_INPUT_DATA, ssl_parse_server_hello(&session, diagnostics));
    EXPECT_EQ(nullptr, diagnosticContext.frame);
    contents = Contents();
    EXPECT_NE(std::string::npos, contents.find("peer selected unsupported ciphersuite 0x1301")) << contents;
    EXPECT_NE(std::string::npos, contents.find("ssl_parse_server_hello failed")) << contents;
}
