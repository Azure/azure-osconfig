// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <Tls.h>
#include <Deadline.h>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

struct ssl_st {};
struct ssl_ctx_st {};
struct ssl_method_st {};
struct x509_st {};
struct X509_VERIFY_PARAM_st {};
struct x509_store_ctx_st;
struct ossl_init_settings_st;

namespace
{
// This suite uses a separately named copy of the real C adapter with mocked
// dlopen/dlsym. Source-local definitions leave other commontests suites and
// production builds using the real adapter and system loader.
// The fake OpenSSL functions below record calls and inject results; they perform
// no cryptography or certificate validation. These cases test adapter decisions,
// not TLS interoperability. TelemetryTlsUT.cpp covers real OpenSSL with local peers.
struct Provider
{
    const char* name;
    unsigned long version;
    bool available;
};

Provider providers[] = {
    {"libssl.so.3", 0x30000000UL, true},
    {"libssl.so.1.1", 0x10101000UL, true},
    {"libssl.so.1.0.2", 0x1000215fUL, true},
    {"libssl.so.10", 0x100020bfUL, true},
    {"libssl.so.1.0.0", 0x1000200fUL, true}
};
Provider* current = nullptr;
std::vector<std::string> opened;
std::string missingSymbol, hostname, ipAddress, serverName, configurationFile;
int missingProvider = -1;
int initializeResult = 1, configurationResult = 1, trustResult = 1;
int connectResult = 1;
int initialized = 0, configured = 0, modules = 0, engines = 0, contextCount = 0;
long minimum = 0, verificationResult = 0;
unsigned long configurationFlags = 0;
unsigned int hostFlags = 0;
uint64_t options = 0x80;
bool configWasNull = false;
ssl_ctx_st context;
ssl_st connection;
ssl_method_st modernMethod, legacyMethod;
const ssl_method_st* selectedMethod = nullptr;
X509_VERIFY_PARAM_st parameters;
x509_st certificate;

unsigned long Version() { return current->version; }
void ClearErrors() {}
unsigned long GetError() { return 0; }
void LoadStrings() {}
void LoadModules() { ++modules; }
void LoadEngines() { ++engines; }

int Initialize(uint64_t flags, const ossl_init_settings_st* settings)
{
    EXPECT_EQ(UINT64_C(0x40), flags);
    EXPECT_EQ(nullptr, settings);
    ++initialized;
    return initializeResult;
}

int InitializeLegacy()
{
    ++initialized;
    return initializeResult;
}

int Configure(const char* file, const char* application, unsigned long flags)
{
    EXPECT_EQ(nullptr, application);
    ++configured;
    configWasNull = nullptr == file;
    configurationFile = file ? file : "";
    configurationFlags = flags;
    return configurationResult;
}

const ssl_method_st* ModernMethod() { return &modernMethod; }
const ssl_method_st* LegacyMethod() { return &legacyMethod; }

ssl_ctx_st* CreateContext(const ssl_method_st* method)
{
    ++contextCount;
    selectedMethod = method;
    return &context;
}

void FreeContext(ssl_ctx_st*) {}

long ContextControl(ssl_ctx_st*, int command, long value, void* pointer)
{
    EXPECT_EQ(nullptr, pointer);

    if (32 == command)
    {
        EXPECT_LT(current->version, 0x10100000UL);
        options |= static_cast<unsigned long>(value);
        return static_cast<long>(options);
    }

    EXPECT_GE(current->version, 0x10100000UL);

    if (130 == command)
    {
        return minimum;
    }

    EXPECT_EQ(123, command);
    minimum = value;
    return 1;
}

void SetVerify(ssl_ctx_st*, int mode, int (*callback)(int, x509_store_ctx_st*))
{
    EXPECT_EQ(1, mode);
    EXPECT_EQ(nullptr, callback);
}

int DefaultTrust(ssl_ctx_st*) { return trustResult; }

int SetAlpn(ssl_ctx_st*, const unsigned char* protocols, unsigned int size)
{
    const unsigned char expected[] = {8, 'h', 't', 't', 'p', '/', '1', '.', '1'};
    EXPECT_EQ(sizeof(expected), size);
    EXPECT_EQ(0, memcmp(protocols, expected, sizeof(expected)));
    return 0;
}

uint64_t SetOptions3(ssl_ctx_st*, uint64_t value) { return options |= value; }
uint64_t ClearOptions3(ssl_ctx_st*, uint64_t value) { return options &= ~value; }
unsigned long SetOptions11(ssl_ctx_st*, unsigned long value)
{
    options |= value;
    return static_cast<unsigned long>(options);
}

ssl_st* CreateConnection(ssl_ctx_st*) { return &connection; }
void FreeConnection(ssl_st*) {}
int SetDescriptor(ssl_st*, int) { return 1; }
X509_VERIFY_PARAM_st* GetParameters(ssl_st*) { return &parameters; }
unsigned int GetHostFlags(const X509_VERIFY_PARAM_st*) { return hostFlags; }
void SetHostFlags(X509_VERIFY_PARAM_st*, unsigned int flags) { hostFlags = flags; }

int SetHost(X509_VERIFY_PARAM_st*, const char* value, size_t length)
{
    EXPECT_EQ(0U, length);
    hostname = value;
    return 1;
}

int SetIp(X509_VERIFY_PARAM_st*, const char* value)
{
    ipAddress = value;
    return 1;
}

long ConnectionControl(ssl_st*, int command, long type, void* value)
{
    EXPECT_EQ(55, command);
    EXPECT_EQ(0, type);
    serverName = static_cast<const char*>(value);
    return 1;
}

int Connect(ssl_st*) { return verificationResult ? -1 : connectResult; }
int ConnectionError(const ssl_st*, int) { return 1; }
long VerifyResult(const ssl_st*) { return verificationResult; }
void GetAlpn(const ssl_st*, const unsigned char** protocol, unsigned int* size)
{
    *protocol = reinterpret_cast<const unsigned char*>("http/1.1");
    *size = 8;
}

x509_st* PeerCertificate(const ssl_st*) { return &certificate; }
void FreeCertificate(x509_st*) {}
int Write(ssl_st*, const void*, int size) { return size; }
int Read(ssl_st*, void*, int) { return -1; }

template<typename Function>
void* Address(Function function)
{
    void* address = nullptr;
    static_assert(sizeof(address) == sizeof(function), "Unsupported test ABI");
    memcpy(&address, &function, sizeof(address));
    return address;
}

int64_t Deadline()
{
    int64_t now = 0;
    EXPECT_EQ(0, TelemetryMonotonicTime(&now));
    return now + INT64_C(5000000000);
}

void Prepare(size_t first)
{
    struct sigaction action = {};
    action.sa_handler = SIG_IGN;
    sigemptyset(&action.sa_mask);
    ASSERT_EQ(0, sigaction(SIGPIPE, &action, nullptr));
    ASSERT_EQ(0, unsetenv("OPENSSL_CONF"));

    for (size_t i = 0; i < first; ++i)
    {
        providers[i].available = false;
    }
}

void CreateAndCheck(size_t index, bool exchange = false, bool ip = false)
{
    TelemetryTls* tls = nullptr;
    int descriptor = -1;
    const bool legacy = index >= 2;

    ASSERT_EQ(0, TelemetryTlsCreate(&tls, Deadline(), nullptr));
    ASSERT_NE(nullptr, tls);
    ASSERT_NE(nullptr, current);
    EXPECT_EQ(&providers[index], current);
    EXPECT_EQ(index + 1, opened.size());
    EXPECT_EQ(1, initialized);
    EXPECT_EQ(legacy ? 1 : 0, configured);
    EXPECT_EQ(legacy ? &legacyMethod : &modernMethod, selectedMethod);
    EXPECT_NE(UINT64_C(0), options & UINT64_C(0x20000));

    if (legacy)
    {
        EXPECT_EQ(1, modules);
        EXPECT_EQ(1, engines);
        EXPECT_EQ(0, minimum);
    }
    else
    {
        EXPECT_GE(minimum, 0x0303);
        if (0 == index)
        {
            EXPECT_EQ(UINT64_C(0), options & UINT64_C(0x80));
        }
    }

    if (exchange)
    {
        descriptor = socket(AF_INET, SOCK_STREAM, 0);
        ASSERT_GE(descriptor, 0);
        ASSERT_EQ(0, fcntl(descriptor, F_SETFL, O_NONBLOCK));
        EXPECT_EQ(0, TelemetryTlsHandshake(tls, descriptor,
            ip ? "127.0.0.1" : "example.test", Deadline(), nullptr));
        EXPECT_EQ(ip ? "" : "example.test", hostname);
        EXPECT_EQ(ip ? "127.0.0.1" : "", ipAddress);
        EXPECT_EQ(ip ? "" : "example.test", serverName);
        EXPECT_NE(0U, hostFlags & 4U);
        EXPECT_EQ(0, TelemetryTlsWrite(tls, "test", 4, Deadline(), nullptr));
    }

    TelemetryTlsDestroy(&tls, nullptr);
    EXPECT_EQ(nullptr, tls);

    if (descriptor >= 0)
    {
        EXPECT_GE(fcntl(descriptor, F_GETFL), 0);
        close(descriptor);
    }
}

void ExpectCreateFailure(int status)
{
    TelemetryTls* tls = nullptr;
    EXPECT_EQ(status, TelemetryTlsCreate(&tls, Deadline(), nullptr));
    EXPECT_EQ(nullptr, tls);
    const size_t attempted = opened.size();
    EXPECT_EQ(status, TelemetryTlsCreate(&tls, Deadline(), nullptr));
    EXPECT_EQ(nullptr, tls);
    EXPECT_EQ(attempted, opened.size());
}

void ExpectHandshakeFailure(size_t index, int status)
{
    TelemetryTls* tls = nullptr;
    ASSERT_EQ(0, TelemetryTlsCreate(&tls, Deadline(), nullptr));
    const int descriptor = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(descriptor, 0);
    ASSERT_EQ(0, fcntl(descriptor, F_SETFL, O_NONBLOCK));
    EXPECT_EQ(status, TelemetryTlsHandshake(tls, descriptor, "example.test", Deadline(), nullptr));
    EXPECT_EQ(EINVAL, TelemetryTlsWrite(tls, "x", 1, Deadline(), nullptr));
    EXPECT_EQ(index + 1, opened.size());
    TelemetryTlsDestroy(&tls, nullptr);
    close(descriptor);
}
}

// Test-only loader: return a fixture record, not an actual shared-library handle.
extern "C" void* TelemetryTestDlopen(const char* name, int) noexcept
{
    opened.push_back(name);

    for (Provider& provider : providers)
    {
        if ((provider.available) && (0 == strcmp(name, provider.name)))
        {
            return &provider;
        }
    }

    return nullptr;
}

// Expose only the selected version's simulated API, with optional missing symbols
// for failure cases. Returned addresses refer exclusively to test functions above.
extern "C" void* TelemetryTestDlsym(void* library, const char* name) noexcept
{
    current = static_cast<Provider*>(library);
    const bool legacy = current->version < 0x10100000UL;

    if ((missingProvider >= 0) && (current == &providers[missingProvider]) && (missingSymbol == name))
    {
        return nullptr;
    }

#define SYMBOL(symbol, function) if (0 == strcmp(name, symbol)) { return Address(function); }
    if (legacy)
    {
        SYMBOL("SSLeay", Version);
        SYMBOL("SSL_library_init", InitializeLegacy);
        SYMBOL("SSL_load_error_strings", LoadStrings);
        SYMBOL("OPENSSL_load_builtin_modules", LoadModules);
        SYMBOL("ENGINE_load_builtin_engines", LoadEngines);
        SYMBOL("CONF_modules_load_file", Configure);
        SYMBOL("TLSv1_2_client_method", LegacyMethod);
        SYMBOL("SSL_get_peer_certificate", PeerCertificate);
    }
    else
    {
        SYMBOL("OpenSSL_version_num", Version);
        SYMBOL("OPENSSL_init_ssl", Initialize);
        SYMBOL("TLS_client_method", ModernMethod);
        SYMBOL("X509_VERIFY_PARAM_get_hostflags", GetHostFlags);
        if (current->version >= 0x30000000UL)
        {
            SYMBOL("SSL_CTX_set_options", SetOptions3);
            SYMBOL("SSL_CTX_clear_options", ClearOptions3);
            SYMBOL("SSL_get1_peer_certificate", PeerCertificate);
        }
        else
        {
            SYMBOL("SSL_CTX_set_options", SetOptions11);
            SYMBOL("SSL_get_peer_certificate", PeerCertificate);
        }
    }
    SYMBOL("SSL_CTX_new", CreateContext);
    SYMBOL("SSL_CTX_free", FreeContext);
    SYMBOL("SSL_CTX_ctrl", ContextControl);
    SYMBOL("SSL_CTX_set_verify", SetVerify);
    SYMBOL("SSL_CTX_set_default_verify_paths", DefaultTrust);
    SYMBOL("SSL_CTX_set_alpn_protos", SetAlpn);
    SYMBOL("SSL_new", CreateConnection);
    SYMBOL("SSL_free", FreeConnection);
    SYMBOL("SSL_set_fd", SetDescriptor);
    SYMBOL("SSL_get0_param", GetParameters);
    SYMBOL("X509_VERIFY_PARAM_set1_host", SetHost);
    SYMBOL("X509_VERIFY_PARAM_set_hostflags", SetHostFlags);
    SYMBOL("X509_VERIFY_PARAM_set1_ip_asc", SetIp);
    SYMBOL("SSL_ctrl", ConnectionControl);
    SYMBOL("SSL_connect", Connect);
    SYMBOL("SSL_get_error", ConnectionError);
    SYMBOL("SSL_get_verify_result", VerifyResult);
    SYMBOL("SSL_get0_alpn_selected", GetAlpn);
    SYMBOL("X509_free", FreeCertificate);
    SYMBOL("SSL_write", Write);
    SYMBOL("SSL_read", Read);
    SYMBOL("ERR_clear_error", ClearErrors);
    SYMBOL("ERR_get_error", GetError);
#undef SYMBOL
    return nullptr;
}

#define PROVIDER_CASE(statement) EXPECT_EXIT( \
    { statement; _exit(::testing::Test::HasFailure() ? 1 : 0); }, \
    ::testing::ExitedWithCode(0), "")

TEST(TelemetryTlsProviderDeathTest, Prefers3AndPreservesStricterPolicy)
{
    PROVIDER_CASE({
        Prepare(0);
        minimum = 0x0304;
        hostFlags = 2;
        CreateAndCheck(0, true);
        EXPECT_EQ(0x0304, minimum);
        EXPECT_EQ(6U, hostFlags);
    });
}

TEST(TelemetryTlsProviderDeathTest, Uses11Before102)
{
    PROVIDER_CASE({ Prepare(1); CreateAndCheck(1, true); });
}

TEST(TelemetryTlsProviderDeathTest, Uses102ApisAndVerifiesDns)
{
    PROVIDER_CASE({
        Prepare(2);
        CreateAndCheck(2, true);
        EXPECT_TRUE(configWasNull);
        EXPECT_EQ(0x30UL, configurationFlags);
    });
}

TEST(TelemetryTlsProviderDeathTest, SupportsEl7SonameAndVerifiesIpWithoutSni)
{
    PROVIDER_CASE({ Prepare(3); CreateAndCheck(3, true, true); });
}

TEST(TelemetryTlsProviderDeathTest, SupportsDebianUbuntuSoname)
{
    PROVIDER_CASE({ Prepare(4); CreateAndCheck(4, true); });
}

TEST(TelemetryTlsProviderDeathTest, Rejects101EvenWithLegacySoname)
{
    PROVIDER_CASE({
        Prepare(4);
        providers[4].version = 0x100010ffUL;
        ExpectCreateFailure(ENOTSUP);
        EXPECT_EQ(0, initialized);
    });
}

TEST(TelemetryTlsProviderDeathTest, RejectsUnexpectedMajorVersion)
{
    PROVIDER_CASE({
        Prepare(0);
        providers[0].version = 0x40000000UL;
        for (size_t i = 1; i < 5; ++i) { providers[i].available = false; }
        ExpectCreateFailure(ENOTSUP);
        EXPECT_EQ(0, initialized);
    });
}

TEST(TelemetryTlsProviderDeathTest, ReportsMissingRuntimeWithoutFallback)
{
    PROVIDER_CASE({
        Prepare(5);
        ExpectCreateFailure(ENOTSUP);
        EXPECT_EQ(5U, opened.size());
        EXPECT_EQ(0, contextCount);
    });
}

TEST(TelemetryTlsProviderDeathTest, SkipsIncompleteApiBeforeInitialization)
{
    PROVIDER_CASE({
        Prepare(0);
        missingProvider = 0;
        missingSymbol = "X509_VERIFY_PARAM_set1_host";
        SetConsoleLoggingEnabled(true);
        SetLoggingLevel(LoggingLevelDebug);
        ::testing::internal::CaptureStdout();
        CreateAndCheck(1);
        EXPECT_TRUE(::testing::internal::GetCapturedStdout().empty());
    });
}

TEST(TelemetryTlsProviderDeathTest, SkipsIncompatibleVersionBeforeInitialization)
{
    PROVIDER_CASE({
        Prepare(0);
        providers[0].version = 0x40000000UL;
        SetConsoleLoggingEnabled(true);
        SetLoggingLevel(LoggingLevelDebug);
        ::testing::internal::CaptureStdout();
        CreateAndCheck(1);
        EXPECT_TRUE(::testing::internal::GetCapturedStdout().empty());
    });
}

TEST(TelemetryTlsProviderDeathTest, RejectsLegacyProviderWithoutIdentityVerification)
{
    PROVIDER_CASE({
        Prepare(4);
        missingProvider = 4;
        missingSymbol = "X509_VERIFY_PARAM_set1_ip_asc";
        ExpectCreateFailure(ENOTSUP);
        EXPECT_EQ(0, initialized);
    });
}

TEST(TelemetryTlsProviderDeathTest, DoesNotDowngradeAfterModernInitializationFailure)
{
    PROVIDER_CASE({
        Prepare(0);
        initializeResult = 0;
        ExpectCreateFailure(EIO);
        EXPECT_EQ(1U, opened.size());
        EXPECT_EQ(1, initialized);
    });
}

TEST(TelemetryTlsProviderDeathTest, DoesNotTryAnotherLegacyLibraryAfterInitializationFailure)
{
    PROVIDER_CASE({
        Prepare(2);
        initializeResult = 0;
        ExpectCreateFailure(EIO);
        EXPECT_EQ(3U, opened.size());
        EXPECT_EQ(0, configured);
    });
}

TEST(TelemetryTlsProviderDeathTest, FailsOnLegacyConfigurationErrorWithoutDowngrade)
{
    PROVIDER_CASE({
        Prepare(2);
        configurationResult = 0;
        ASSERT_EQ(0, setenv("OPENSSL_CONF", "/fixture/invalid.cnf", 1));
        ExpectCreateFailure(EIO);
        EXPECT_EQ(3U, opened.size());
        EXPECT_EQ(1, configured);
        EXPECT_FALSE(configWasNull);
        EXPECT_EQ("/fixture/invalid.cnf", configurationFile);
        EXPECT_EQ(0x20UL, configurationFlags);
        EXPECT_EQ(0, contextCount);
    });
}

TEST(TelemetryTlsProviderDeathTest, LoadsExplicitLegacyConfigurationWithoutIgnoringMissingFile)
{
    PROVIDER_CASE({
        Prepare(2);
        ASSERT_EQ(0, setenv("OPENSSL_CONF", "/fixture/policy.cnf", 1));
        CreateAndCheck(2);
        EXPECT_EQ("/fixture/policy.cnf", configurationFile);
        EXPECT_EQ(0x20UL, configurationFlags);
    });
}

TEST(TelemetryTlsProviderDeathTest, SupportsLegacyBuildWithoutEngines)
{
    PROVIDER_CASE({
        Prepare(2);
        missingProvider = 2;
        missingSymbol = "ENGINE_load_builtin_engines";
        TelemetryTls* tls = nullptr;
        ASSERT_EQ(0, TelemetryTlsCreate(&tls, Deadline(), nullptr));
        EXPECT_EQ(1, configured);
        EXPECT_EQ(0, engines);
        EXPECT_EQ(3U, opened.size());
        TelemetryTlsDestroy(&tls, nullptr);
    });
}

TEST(TelemetryTlsProviderDeathTest, DoesNotDowngradeAfterTrustSetupFailure)
{
    PROVIDER_CASE({
        Prepare(0);
        trustResult = 0;
        ExpectCreateFailure(EIO);
        EXPECT_EQ(1U, opened.size());
        EXPECT_EQ(1, initialized);
    });
}

TEST(TelemetryTlsProviderDeathTest, DoesNotDowngradeAfterCertificateFailure)
{
    PROVIDER_CASE({
        Prepare(0);
        verificationResult = 62;
        ExpectHandshakeFailure(0, EACCES);
    });
}

TEST(TelemetryTlsProviderDeathTest, DoesNotTryAnotherLegacyLibraryAfterCertificateFailure)
{
    PROVIDER_CASE({
        Prepare(2);
        verificationResult = 62;
        ExpectHandshakeFailure(2, EACCES);
    });
}

TEST(TelemetryTlsProviderDeathTest, DoesNotDowngradeAfterProtocolFailure)
{
    PROVIDER_CASE({
        Prepare(0);
        connectResult = -1;
        ExpectHandshakeFailure(0, EPROTO);
    });
}
