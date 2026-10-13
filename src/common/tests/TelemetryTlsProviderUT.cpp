// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <Tls.h>
#include <Deadline.h>
#include <arpa/inet.h>
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
struct asn1_string_st
{
    int type;
    std::string data;
};
struct GENERAL_NAME_st
{
    int type;
    asn1_string_st value;
};
struct stack_st {};
struct stack_st_GENERAL_NAME
{
    std::vector<GENERAL_NAME_st> values;
};
struct X509_name_st {};
struct X509_name_entry_st {};

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
    {"libssl.so.1.0.0", 0x1000200fUL, true},
    {"libssl.so.1.0.1", 0x100010ffUL, false}
};
Provider* current = nullptr;
std::vector<std::string> opened;
std::string missingSymbol, hostname, ipAddress, serverName, configurationFile;
int missingProvider = -1;
int initializeResult = 1, configurationResult = 1, trustResult = 1;
int connectResult = 1;
int connectionError = 1, peekResult = -1, peeks = 0;
int peekDescriptor = -1;
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
stack_st_GENERAL_NAME alternativeNames = {{{2, {22, "example.test"}}}};
X509_name_st subject;
X509_name_entry_st commonNameEntry;
asn1_string_st commonName = {12, "example.test"};
int sanCritical = 0, commonNameCount = 1, identityChecks = 0, namesFreed = 0;
int alpnConfigured = 0, alpnRead = 0, writes = 0;
bool sanAvailable = true, conversionFails = false;
bool peerPresent = true;

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
    ++alpnConfigured;
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
int ConnectionError(const ssl_st*, int) { return connectionError; }
long VerifyResult(const ssl_st*) { return verificationResult; }
void GetAlpn(const ssl_st*, const unsigned char** protocol, unsigned int* size)
{
    ++alpnRead;
    *protocol = reinterpret_cast<const unsigned char*>("http/1.1");
    *size = 8;
}

x509_st* PeerCertificate(const ssl_st*) { return peerPresent ? &certificate : nullptr; }
void FreeCertificate(x509_st*) {}
int Write(ssl_st*, const void*, int size) { ++writes; return size; }
int Read(ssl_st*, void*, int) { return -1; }
int Peek(ssl_st*, void*, int)
{
    ++peeks;
    if (peekDescriptor >= 0)
    {
        char control = 0;
        EXPECT_EQ(1, recv(peekDescriptor, &control, 1, MSG_DONTWAIT));
    }
    return peekResult;
}

void* GetAlternativeNames(x509_st*, int nid, int* critical, int* index)
{
    EXPECT_EQ(85, nid);
    EXPECT_EQ(nullptr, index);
    ++identityChecks;
    *critical = sanCritical;
    return sanAvailable ? &alternativeNames : nullptr;
}

int StackCount(const stack_st* values)
{
    return static_cast<int>(reinterpret_cast<const stack_st_GENERAL_NAME*>(values)->values.size());
}

void* StackValue(const stack_st*, int index) { return &alternativeNames.values.at(index); }
void* NameValue(GENERAL_NAME_st* name, int* type) { *type = name->type; return &name->value; }
void FreeNames(stack_st_GENERAL_NAME*) { ++namesFreed; }
int StringType(asn1_string_st* value) { return value->type; }
int StringLength(const asn1_string_st* value) { return static_cast<int>(value->data.size()); }
unsigned char* StringData(asn1_string_st* value) { return reinterpret_cast<unsigned char*>(&value->data[0]); }

int StringToUtf8(unsigned char** output, asn1_string_st* value)
{
    int length = -1;
    *output = nullptr;
    if (!conversionFails)
    {
        *output = static_cast<unsigned char*>(malloc(value->data.size() + 1));
        if (nullptr != *output)
        {
            memcpy(*output, value->data.c_str(), value->data.size() + 1);
            length = static_cast<int>(value->data.size());
        }
    }
    return length;
}

X509_name_st* SubjectName(x509_st*) { return &subject; }
int CommonNameIndex(X509_name_st*, int nid, int previous)
{
    EXPECT_EQ(13, nid);
    return previous + 1 < commonNameCount ? previous + 1 : -1;
}
X509_name_entry_st* NameEntry(X509_name_st*, int) { return &commonNameEntry; }
asn1_string_st* EntryData(X509_name_entry_st*) { return &commonName; }
void CryptoFree(void* value) { free(value); }

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
    const bool oldest = providers[index].version < 0x10002000UL;

    ASSERT_EQ(0, TelemetryTlsCreate(&tls, Deadline(), nullptr));
    ASSERT_NE(nullptr, tls);
    ASSERT_NE(nullptr, current);
    EXPECT_EQ(&providers[index], current);
    EXPECT_EQ(oldest && index < 5 ? index + 4 : index + 1, opened.size());
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
        EXPECT_EQ(oldest || ip ? "" : "example.test", hostname);
        EXPECT_EQ(!oldest && ip ? "127.0.0.1" : "", ipAddress);
        EXPECT_EQ(ip ? "" : "example.test", serverName);
        EXPECT_EQ(oldest ? 0U : 4U, hostFlags & 4U);
        EXPECT_EQ(oldest ? 1 : 0, identityChecks);
        EXPECT_EQ(oldest ? 0 : 1, alpnRead);
        EXPECT_EQ(0, TelemetryTlsWrite(tls, "test", 4, Deadline(), nullptr));
    }
    EXPECT_EQ(oldest ? 0 : 1, alpnConfigured);

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

void Prepare101()
{
    Prepare(5);
    providers[5].available = true;
}

void Check101Identity(const char* peer, int expected)
{
    TelemetryTls* tls = nullptr;
    ASSERT_EQ(0, TelemetryTlsCreate(&tls, Deadline(), nullptr));
    const int descriptor = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(descriptor, 0);
    ASSERT_EQ(0, fcntl(descriptor, F_SETFL, O_NONBLOCK));
    EXPECT_EQ(expected, TelemetryTlsHandshake(tls, descriptor, peer, Deadline(), nullptr));
    EXPECT_EQ(expected ? EINVAL : 0, TelemetryTlsWrite(tls, "test", 4, Deadline(), nullptr));
    EXPECT_EQ(expected ? 0 : 1, writes);
    EXPECT_EQ(6U, opened.size());
    EXPECT_EQ(0, alpnConfigured);
    EXPECT_EQ(0, alpnRead);
    EXPECT_EQ(sanAvailable ? 1 : 0, namesFreed);
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
    SYMBOL("SSL_new", CreateConnection);
    SYMBOL("SSL_free", FreeConnection);
    SYMBOL("SSL_set_fd", SetDescriptor);
    if (current->version >= 0x10002000UL)
    {
        SYMBOL("SSL_CTX_set_alpn_protos", SetAlpn);
        SYMBOL("SSL_get0_alpn_selected", GetAlpn);
        SYMBOL("SSL_get0_param", GetParameters);
        SYMBOL("X509_VERIFY_PARAM_set1_host", SetHost);
        SYMBOL("X509_VERIFY_PARAM_set_hostflags", SetHostFlags);
        SYMBOL("X509_VERIFY_PARAM_set1_ip_asc", SetIp);
    }
    else
    {
        SYMBOL("X509_get_ext_d2i", GetAlternativeNames);
        SYMBOL("sk_num", StackCount);
        SYMBOL("sk_value", StackValue);
        SYMBOL("GENERAL_NAME_get0_value", NameValue);
        SYMBOL("GENERAL_NAMES_free", FreeNames);
        SYMBOL("ASN1_STRING_type", StringType);
        SYMBOL("ASN1_STRING_length", StringLength);
        SYMBOL("ASN1_STRING_data", StringData);
        SYMBOL("ASN1_STRING_to_UTF8", StringToUtf8);
        SYMBOL("X509_get_subject_name", SubjectName);
        SYMBOL("X509_NAME_get_index_by_NID", CommonNameIndex);
        SYMBOL("X509_NAME_get_entry", NameEntry);
        SYMBOL("X509_NAME_ENTRY_get_data", EntryData);
        SYMBOL("CRYPTO_free", CryptoFree);
    }
    SYMBOL("SSL_ctrl", ConnectionControl);
    SYMBOL("SSL_connect", Connect);
    SYMBOL("SSL_get_error", ConnectionError);
    SYMBOL("SSL_get_verify_result", VerifyResult);
    SYMBOL("X509_free", FreeCertificate);
    SYMBOL("SSL_write", Write);
    SYMBOL("SSL_read", Read);
    SYMBOL("SSL_peek", Peek);
    SYMBOL("ERR_clear_error", ClearErrors);
    SYMBOL("ERR_get_error", GetError);
#undef SYMBOL
    return nullptr;
}

#define PROVIDER_CASE(...) EXPECT_EXIT( \
    ([&]() { __VA_ARGS__; _exit(::testing::Test::HasFailure() ? 1 : 0); }()), \
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

TEST(TelemetryTlsProviderDeathTest, Uses101OnlyAfterAllNewerCandidates)
{
    PROVIDER_CASE({
        Prepare(4);
        providers[4].version = 0x100010ffUL;
        CreateAndCheck(4, true);
    });
}

TEST(TelemetryTlsProviderDeathTest, RejectsUnexpectedMajorVersion)
{
    PROVIDER_CASE({
        Prepare(0);
        providers[0].version = 0x40000000UL;
        for (size_t i = 1; i < ARRAY_SIZE(providers); ++i) { providers[i].available = false; }
        ExpectCreateFailure(ENOTSUP);
        EXPECT_EQ(0, initialized);
    });
}

TEST(TelemetryTlsProviderDeathTest, ReportsMissingRuntimeWithoutFallback)
{
    PROVIDER_CASE({
        Prepare(ARRAY_SIZE(providers));
        ExpectCreateFailure(ENOTSUP);
        EXPECT_EQ(8U, opened.size());
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

TEST(TelemetryTlsProviderDeathTest, InvalidPeerIsNotMaskedByClosedDescriptor)
{
    PROVIDER_CASE({
        Prepare(0);
        TelemetryTls* tls = nullptr;
        ASSERT_EQ(0, TelemetryTlsCreate(&tls, Deadline(), nullptr));
        const int descriptor = socket(AF_INET, SOCK_STREAM, 0);
        ASSERT_GE(descriptor, 0);
        ASSERT_EQ(0, close(descriptor));
        EXPECT_EQ(EINVAL, TelemetryTlsHandshake(tls, descriptor, "bad\nname", Deadline(), nullptr));
        EXPECT_EQ(EBADF, TelemetryTlsHandshake(tls, descriptor, "example.test", Deadline(), nullptr));
        EXPECT_EQ(EINVAL, TelemetryTlsWrite(tls, "x", 1, Deadline(), nullptr));
        TelemetryTlsDestroy(&tls, nullptr);
    });
}

TEST(TelemetryTlsProviderDeathTest, IdleProbeKeepsWantReadButRetiresClosedOrUnexpectedData)
{
    PROVIDER_CASE({
        Prepare101();
        for (const int error : {2, 3, 5, 6, 1, 0})
        {
            TelemetryTls* tls = nullptr;
            ASSERT_EQ(0, TelemetryTlsCreate(&tls, Deadline(), nullptr));
            int sockets[2] = {-1, -1};
            ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sockets));
            const int descriptor = sockets[0];
            ASSERT_EQ(0, fcntl(descriptor, F_SETFL, O_NONBLOCK));
            ASSERT_EQ(0, TelemetryTlsHandshake(tls, descriptor, "example.test", Deadline(), nullptr));
            connectionError = error;
            peekResult = error ? -1 : 1;
            peeks = 0;
            bool reusable = true;
            EXPECT_EQ(0, TelemetryTlsCheckIdle(tls, &reusable, Deadline(), nullptr));
            EXPECT_EQ(2 == error, reusable);
            EXPECT_EQ(1, peeks);
            EXPECT_EQ(2 == error ? 0 : EINVAL, TelemetryTlsWrite(tls, "x", 1, Deadline(), nullptr));
            EXPECT_GE(fcntl(descriptor, F_GETFL), 0);
            TelemetryTlsDestroy(&tls, nullptr);
            ASSERT_EQ(0, close(descriptor));
            ASSERT_EQ(0, close(sockets[1]));
        }
        EXPECT_EQ(1, initialized);
        EXPECT_EQ(6U, opened.size());
    });
}

TEST(TelemetryTlsProviderDeathTest, IdleProbeDrainsReadyControlRecordsAndDetectsFollowingClose)
{
    PROVIDER_CASE({
        Prepare101();
        TelemetryTls* tls = nullptr;
        int sockets[2] = {-1, -1};
        ASSERT_EQ(0, TelemetryTlsCreate(&tls, Deadline(), nullptr));
        ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sockets));
        ASSERT_EQ(0, fcntl(sockets[0], F_SETFL, O_NONBLOCK));
        ASSERT_EQ(0, TelemetryTlsHandshake(tls, sockets[0], "example.test", Deadline(), nullptr));
        connectionError = 2;
        peekDescriptor = sockets[0];
        ASSERT_EQ(2, send(sockets[1], "ab", 2, MSG_NOSIGNAL));
        bool reusable = false;
        EXPECT_EQ(0, TelemetryTlsCheckIdle(tls, &reusable, Deadline(), nullptr));
        EXPECT_TRUE(reusable);
        EXPECT_EQ(2, peeks);
        EXPECT_EQ(0, writes);

        ASSERT_EQ(1, send(sockets[1], "c", 1, MSG_NOSIGNAL));
        ASSERT_EQ(0, close(sockets[1]));
        EXPECT_EQ(0, TelemetryTlsCheckIdle(tls, &reusable, Deadline(), nullptr));
        EXPECT_FALSE(reusable);
        EXPECT_EQ(3, peeks);
        EXPECT_EQ(EINVAL, TelemetryTlsWrite(tls, "x", 1, Deadline(), nullptr));
        TelemetryTlsDestroy(&tls, nullptr);
        ASSERT_EQ(0, close(sockets[0]));
    });
}

TEST(TelemetryTlsProviderDeathTest, IdleProbeHonorsDeadlineWithoutTouchingProvider)
{
    PROVIDER_CASE({
        Prepare(0);
        TelemetryTls* tls = nullptr;
        bool reusable = true;
        EXPECT_EQ(EINVAL, TelemetryTlsCheckIdle(nullptr, &reusable, Deadline(), nullptr));
        EXPECT_FALSE(reusable);
        ASSERT_EQ(0, TelemetryTlsCreate(&tls, Deadline(), nullptr));
        const int descriptor = socket(AF_INET, SOCK_STREAM, 0);
        ASSERT_GE(descriptor, 0);
        ASSERT_EQ(0, fcntl(descriptor, F_SETFL, O_NONBLOCK));
        ASSERT_EQ(0, TelemetryTlsHandshake(tls, descriptor, "example.test", Deadline(), nullptr));
        EXPECT_EQ(EINVAL, TelemetryTlsCheckIdle(tls, nullptr, Deadline(), nullptr));
        EXPECT_EQ(ETIMEDOUT, TelemetryTlsCheckIdle(tls, &reusable, 0, nullptr));
        EXPECT_FALSE(reusable);
        EXPECT_EQ(0, peeks);
        EXPECT_EQ(EINVAL, TelemetryTlsWrite(tls, "x", 1, Deadline(), nullptr));
        TelemetryTlsDestroy(&tls, nullptr);
        ASSERT_EQ(0, close(descriptor));
    });
}

TEST(TelemetryTlsProviderDeathTest, UsesExplicit101Soname)
{
    PROVIDER_CASE({ Prepare101(); CreateAndCheck(5, true); });
}

TEST(TelemetryTlsProviderDeathTest, Prefers102OnAnotherSonameOver101)
{
    PROVIDER_CASE({
        Prepare(3);
        providers[3].version = 0x1000105fUL;
        CreateAndCheck(4, true);
        EXPECT_EQ(0, identityChecks);
    });
}

TEST(TelemetryTlsProviderDeathTest, Supports101El7Soname)
{
    PROVIDER_CASE({
        Prepare(3);
        providers[3].version = 0x1000105fUL;
        providers[4].available = false;
        CreateAndCheck(3, true);
    });
}

TEST(TelemetryTlsProviderDeathTest, Rejects100AndIncomplete101)
{
    PROVIDER_CASE({
        Prepare101();
        providers[5].version = 0x100000ffUL;
        ExpectCreateFailure(ENOTSUP);
        EXPECT_EQ(0, initialized);
    });
    for (const char* symbol : {"X509_get_ext_d2i", "GENERAL_NAME_get0_value", "GENERAL_NAMES_free",
        "sk_num", "sk_value", "ASN1_STRING_data", "ASN1_STRING_length", "ASN1_STRING_type",
        "ASN1_STRING_to_UTF8", "X509_get_subject_name", "X509_NAME_get_index_by_NID",
        "X509_NAME_get_entry", "X509_NAME_ENTRY_get_data", "CRYPTO_free"})
    {
        PROVIDER_CASE({
            Prepare101();
            missingProvider = 5;
            missingSymbol = symbol;
            ExpectCreateFailure(ENOTSUP);
            EXPECT_EQ(0, initialized);
        });
    }
}

TEST(TelemetryTlsProviderDeathTest, Verifies101DnsNamesAndWildcards)
{
    struct Case { const char* name; const char* peer; int status; };
    const Case cases[] = {
        {"EXAMPLE.TEST", "example.test", 0},
        {"example.test.", "example.test", 0},
        {"example.test", "example.test.", 0},
        {"*.example.test", "host.example.test", 0},
        {"wrong.test", "example.test", EACCES},
        {"example.test.attacker", "example.test", EACCES},
        {"*.example.test", "example.test", EACCES},
        {"*.example.test", "a.b.example.test", EACCES},
        {"*.test", "example.test", EACCES},
        {"ex*.example.test", "example.example.test", EACCES},
        {"*.*.test", "a.example.test", EACCES},
        {"*.example.test", "xn--host.example.test", EACCES},
        {"example..test", "example..test", EACCES},
        {"", "example.test", EACCES}
    };
    for (const Case& item : cases)
    {
        PROVIDER_CASE({
            Prepare101();
            alternativeNames.values[0].value.data = item.name;
            Check101Identity(item.peer, item.status);
        });
    }
    PROVIDER_CASE({
        Prepare101();
        alternativeNames.values[0].value.data = "wrong.test";
        alternativeNames.values.push_back({2, {22, "example.test"}});
        Check101Identity("example.test", 0);
    });
}

TEST(TelemetryTlsProviderDeathTest, Verifies101IpSanWithoutDnsOrCommonNameFallback)
{
    for (const char* peer : {"127.0.0.1", "::1"})
    {
        for (bool match : {false, true})
        {
            PROVIDER_CASE({
                Prepare101();
                unsigned char address[16] = {};
                const bool ipv6 = nullptr != strchr(peer, ':');
                ASSERT_EQ(1, inet_pton(ipv6 ? AF_INET6 : AF_INET, peer, address));
                if (!match) { address[0] ^= 1; }
                alternativeNames.values = {{7, {4, std::string(reinterpret_cast<char*>(address), ipv6 ? 16 : 4)}}};
                Check101Identity(peer, match ? 0 : EACCES);
                EXPECT_TRUE(serverName.empty());
            });
        }
        PROVIDER_CASE({
            Prepare101();
            commonName.data = peer;
            alternativeNames.values[0].value.data = peer;
            Check101Identity(peer, EACCES);
        });
        PROVIDER_CASE({
            Prepare101();
            sanAvailable = false;
            sanCritical = -1;
            commonName.data = peer;
            Check101Identity(peer, EACCES);
        });
    }
}

TEST(TelemetryTlsProviderDeathTest, Uses101CommonNameOnlyWithoutDnsSan)
{
    PROVIDER_CASE({
        Prepare101();
        sanAvailable = false;
        sanCritical = -1;
        Check101Identity("example.test", 0);
    });
    PROVIDER_CASE({
        Prepare101();
        alternativeNames.values = {{6, {22, "https://example.test"}}};
        Check101Identity("example.test", 0);
    });
    PROVIDER_CASE({
        Prepare101();
        alternativeNames.values[0].value.data = "wrong.test";
        Check101Identity("example.test", EACCES);
    });
    for (int count : {0, 2})
    {
        PROVIDER_CASE({
            Prepare101();
            sanAvailable = false;
            sanCritical = -1;
            commonNameCount = count;
            Check101Identity("example.test", EACCES);
        });
    }
    PROVIDER_CASE({
        Prepare101();
        sanAvailable = false;
        sanCritical = -1;
        conversionFails = true;
        Check101Identity("example.test", EACCES);
    });
}

TEST(TelemetryTlsProviderDeathTest, RejectsMalformed101Identities)
{
    for (int critical : {-2, 0, 1})
    {
        PROVIDER_CASE({
            Prepare101();
            sanAvailable = false;
            sanCritical = critical;
            Check101Identity("example.test", EACCES);
        });
    }
    PROVIDER_CASE({
        Prepare101();
        alternativeNames.values.clear();
        Check101Identity("example.test", EACCES);
    });
    PROVIDER_CASE({
        Prepare101();
        alternativeNames.values[0].value.data = std::string("example.test\0.attacker", 22);
        Check101Identity("example.test", EACCES);
    });
    PROVIDER_CASE({
        Prepare101();
        alternativeNames.values[0].value.type = 12;
        Check101Identity("example.test", EACCES);
    });
    PROVIDER_CASE({
        Prepare101();
        alternativeNames.values.push_back({7, {4, "bad"}});
        Check101Identity("example.test", EACCES);
    });
    for (const std::string& name : {std::string("example.test\0.attacker", 22), std::string(255, 'a')})
    {
        PROVIDER_CASE({
            Prepare101();
            sanAvailable = false;
            sanCritical = -1;
            commonName.data = name;
            Check101Identity("example.test", EACCES);
        });
    }
}

TEST(TelemetryTlsProviderDeathTest, DoesNotDowngradeAfter101Failures)
{
    PROVIDER_CASE({
        Prepare101();
        peerPresent = false;
        ExpectHandshakeFailure(5, EACCES);
        EXPECT_EQ(0, identityChecks);
        EXPECT_EQ(0, writes);
    });
    PROVIDER_CASE({
        Prepare101();
        initializeResult = 0;
        ExpectCreateFailure(EIO);
        EXPECT_EQ(6U, opened.size());
    });
    PROVIDER_CASE({
        Prepare101();
        configurationResult = 0;
        ExpectCreateFailure(EIO);
        EXPECT_EQ(6U, opened.size());
    });
    PROVIDER_CASE({
        Prepare101();
        trustResult = 0;
        ExpectCreateFailure(EIO);
        EXPECT_EQ(6U, opened.size());
    });
    PROVIDER_CASE({
        Prepare101();
        verificationResult = 20;
        ExpectHandshakeFailure(5, EACCES);
        EXPECT_EQ(0, identityChecks);
        EXPECT_EQ(0, writes);
    });
    PROVIDER_CASE({
        Prepare101();
        connectResult = -1;
        ExpectHandshakeFailure(5, EPROTO);
        EXPECT_EQ(0, identityChecks);
    });
}
