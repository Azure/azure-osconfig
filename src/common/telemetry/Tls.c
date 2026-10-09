// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "Tls.h"
#include "Deadline.h"

#include <arpa/inet.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

// Opaque public OpenSSL ABI types; no OpenSSL build-time dependency.
typedef struct ssl_st Ssl;
typedef struct ssl_ctx_st SslContext;
typedef struct ssl_method_st SslMethod;
typedef struct x509_st Certificate;
typedef struct X509_VERIFY_PARAM_st VerifyParameters;
typedef struct x509_store_ctx_st VerifyContext;
typedef struct ossl_init_settings_st InitSettings;

enum
{
    TlsWantRead = 2,
    TlsWantWrite = 3,
    TlsSyscallError = 5,
    TlsClosed = 6,
    TlsSetServerName = 55,
    TlsSetOptions = 32,
    TlsSetMinimumVersion = 123,
    TlsGetMinimumVersion = 130,
    TlsVersion12 = 0x0303,
    TlsVerifyPeer = 1,
    TlsNoPartialWildcards = 4,
    TlsLoadConfiguration = 0x40,
    TlsIgnoreUnexpectedEof = 0x80,
    TlsNoCompression = 0x20000,
    TlsConfigIgnoreMissingFile = 0x10,
    TlsConfigDefaultSection = 0x20
};

typedef struct TlsApi
{
    int (*OPENSSL_init_ssl)(uint64_t, const InitSettings*);
    int (*SSL_library_init)(void);
    void (*SSL_load_error_strings)(void);
    void (*OPENSSL_load_builtin_modules)(void);
    void (*ENGINE_load_builtin_engines)(void);
    int (*CONF_modules_load_file)(const char*, const char*, unsigned long);
    unsigned long (*OpenSSL_version_num)(void);
    const SslMethod* (*TLS_client_method)(void);
    SslContext* (*SSL_CTX_new)(const SslMethod*);
    void (*SSL_CTX_free)(SslContext*);
    long (*SSL_CTX_ctrl)(SslContext*, int, long, void*);
    void (*SSL_CTX_set_verify)(SslContext*, int, int (*)(int, VerifyContext*));
    int (*SSL_CTX_set_default_verify_paths)(SslContext*);
    int (*SSL_CTX_set_alpn_protos)(SslContext*, const unsigned char*, unsigned int);
    uint64_t (*SetOptions3)(SslContext*, uint64_t);
    uint64_t (*ClearOptions3)(SslContext*, uint64_t);
    unsigned long (*SetOptions11)(SslContext*, unsigned long);
    Ssl* (*SSL_new)(SslContext*);
    void (*SSL_free)(Ssl*);
    int (*SSL_set_fd)(Ssl*, int);
    int (*X509_VERIFY_PARAM_set1_host)(VerifyParameters*, const char*, size_t);
    VerifyParameters* (*SSL_get0_param)(Ssl*);
    unsigned int (*X509_VERIFY_PARAM_get_hostflags)(const VerifyParameters*);
    void (*X509_VERIFY_PARAM_set_hostflags)(VerifyParameters*, unsigned int);
    int (*X509_VERIFY_PARAM_set1_ip_asc)(VerifyParameters*, const char*);
    long (*SSL_ctrl)(Ssl*, int, long, void*);
    int (*SSL_connect)(Ssl*);
    int (*SSL_get_error)(const Ssl*, int);
    long (*SSL_get_verify_result)(const Ssl*);
    void (*SSL_get0_alpn_selected)(const Ssl*, const unsigned char**, unsigned int*);
    Certificate* (*GetPeerCertificate)(const Ssl*);
    void (*X509_free)(Certificate*);
    int (*SSL_write)(Ssl*, const void*, int);
    int (*SSL_read)(Ssl*, void*, int);
    void (*ERR_clear_error)(void);
    unsigned long (*ERR_get_error)(void);
    void (*ERR_error_string_n)(unsigned long, char*, size_t);
} TlsApi;

struct TelemetryTls
{
    SslContext* context;
    Ssl* connection;
    int descriptor;
    bool connected;
};

// Private to the exec'd worker. Never unload provider code underneath its
// registered runtime callbacks; the owned worker's exit reclaims these maps.
static TlsApi g_api;
static void* g_libraries[5];
static bool g_attempted = false;
static int g_providerStatus = ENOTSUP;

static int LoadApi(void* library, TlsApi* api, unsigned long minimumVersion, unsigned long maximumVersion, const char** missingSymbol)
{
    void* symbol = NULL;
    void* peer = NULL;
    unsigned long version = 0;

    *missingSymbol = NULL;

#define LOAD_TLS_NAMED(member, name) do \
{ \
    symbol = dlsym(library, name); \
    \
    if (NULL == symbol) \
    { \
        *missingSymbol = name; \
        return ENOSYS; \
    } \
    \
    _Static_assert(sizeof(symbol) == sizeof(api->member), "Unsupported POSIX function pointer ABI"); \
    memcpy(&api->member, &symbol, sizeof(symbol)); \
} while (0)
#define LOAD_TLS_SYMBOL(member) LOAD_TLS_NAMED(member, #member)
    LOAD_TLS_NAMED(OpenSSL_version_num, (minimumVersion < 0x10100000UL) ? "SSLeay" : "OpenSSL_version_num");
    version = api->OpenSSL_version_num();

    if ((version < minimumVersion) || (version >= maximumVersion))
    {
        return ENOTSUP;
    }

    if (version < 0x10100000UL)
    {
        LOAD_TLS_SYMBOL(SSL_library_init);
        LOAD_TLS_SYMBOL(SSL_load_error_strings);
        LOAD_TLS_SYMBOL(OPENSSL_load_builtin_modules);
        LOAD_TLS_SYMBOL(CONF_modules_load_file);
        LOAD_TLS_NAMED(TLS_client_method, "TLSv1_2_client_method");
        symbol = dlsym(library, "ENGINE_load_builtin_engines");
        memcpy(&api->ENGINE_load_builtin_engines, &symbol, sizeof(symbol));

    }
    else
    {
        LOAD_TLS_SYMBOL(OPENSSL_init_ssl);
        LOAD_TLS_SYMBOL(TLS_client_method);
        LOAD_TLS_SYMBOL(X509_VERIFY_PARAM_get_hostflags);
    }
    LOAD_TLS_SYMBOL(SSL_CTX_new);
    LOAD_TLS_SYMBOL(SSL_CTX_free);
    LOAD_TLS_SYMBOL(SSL_CTX_ctrl);
    LOAD_TLS_SYMBOL(SSL_CTX_set_verify);
    LOAD_TLS_SYMBOL(SSL_CTX_set_default_verify_paths);
    LOAD_TLS_SYMBOL(SSL_CTX_set_alpn_protos);
    LOAD_TLS_SYMBOL(SSL_new);
    LOAD_TLS_SYMBOL(SSL_free);
    LOAD_TLS_SYMBOL(SSL_set_fd);
    LOAD_TLS_SYMBOL(X509_VERIFY_PARAM_set1_host);
    LOAD_TLS_SYMBOL(SSL_get0_param);
    LOAD_TLS_SYMBOL(X509_VERIFY_PARAM_set_hostflags);
    LOAD_TLS_SYMBOL(X509_VERIFY_PARAM_set1_ip_asc);
    LOAD_TLS_SYMBOL(SSL_ctrl);
    LOAD_TLS_SYMBOL(SSL_connect);
    LOAD_TLS_SYMBOL(SSL_get_error);
    LOAD_TLS_SYMBOL(SSL_get_verify_result);
    LOAD_TLS_SYMBOL(SSL_get0_alpn_selected);
    LOAD_TLS_SYMBOL(X509_free);
    LOAD_TLS_SYMBOL(SSL_write);
    LOAD_TLS_SYMBOL(SSL_read);
    LOAD_TLS_SYMBOL(ERR_clear_error);
    LOAD_TLS_SYMBOL(ERR_get_error);
    symbol = dlsym(library, "ERR_error_string_n");
    _Static_assert(sizeof(symbol) == sizeof(api->ERR_error_string_n), "Unsupported POSIX function pointer ABI");
    memcpy(&api->ERR_error_string_n, &symbol, sizeof(symbol));

    // The options ABI changed from unsigned long to uint64_t in OpenSSL 3.
    if (version >= 0x30000000UL)
    {
        LOAD_TLS_NAMED(SetOptions3, "SSL_CTX_set_options");
        LOAD_TLS_NAMED(ClearOptions3, "SSL_CTX_clear_options");
    }
    else if (version >= 0x10100000UL)
    {
        LOAD_TLS_NAMED(SetOptions11, "SSL_CTX_set_options");
    }
#undef LOAD_TLS_SYMBOL
#undef LOAD_TLS_NAMED

    peer = dlsym(library, "SSL_get1_peer_certificate");

    if (NULL == peer)
    {
        peer = dlsym(library, "SSL_get_peer_certificate");
    }

    if (NULL == peer)
    {
        *missingSymbol = "SSL_get1_peer_certificate/SSL_get_peer_certificate";
        return ENOSYS;
    }

    _Static_assert(sizeof(peer) == sizeof(api->GetPeerCertificate), "Unsupported POSIX function pointer ABI");
    memcpy(&api->GetPeerCertificate, &peer, sizeof(peer));

    return 0;
}

static int LoadProvider(OsConfigLogHandle log)
{
    static const struct
    {
        const char* name;
        unsigned long minimum;
        unsigned long maximum;
    } providers[] = {
        {"libssl.so.3", 0x30000000UL, 0x40000000UL},
        {"libssl.so.1.1", 0x10100000UL, 0x10200000UL},
        {"libssl.so.1.0.2", 0x10002000UL, 0x10003000UL},
        {"libssl.so.10", 0x10002000UL, 0x10003000UL},
        {"libssl.so.1.0.0", 0x10002000UL, 0x10003000UL}
    };
    size_t i = 0;
    TlsApi api = {0};
    const char* configuration = NULL;
    unsigned long configurationFlags = TlsConfigDefaultSection;
    const char* operation = "OPENSSL_init_ssl";
    const char* rejectedProvider = NULL;
    const char* missingSymbol = NULL;
    int apiStatus = 0;

    if (g_attempted)
    {
        return g_providerStatus;
    }

    g_attempted = true;

    for (i = 0; i < ARRAY_SIZE(providers); ++i)
    {
        api = (TlsApi){0};
        g_libraries[i] = dlopen(providers[i].name, RTLD_NOW | RTLD_LOCAL);

        if (NULL == g_libraries[i])
        {
            continue;
        }

        if (0 != (apiStatus = LoadApi(g_libraries[i], &api, providers[i].minimum, providers[i].maximum, &missingSymbol)))
        {
            rejectedProvider = providers[i].name;
            continue;
        }

        g_api = api;

        if (NULL != g_api.OPENSSL_init_ssl)
        {
            g_providerStatus = (1 == g_api.OPENSSL_init_ssl(TlsLoadConfiguration, NULL)) ? 0 : EIO;
        }
        else
        {
            operation = "SSL_library_init";
            g_api.SSL_load_error_strings();
            g_providerStatus = (1 == g_api.SSL_library_init()) ? 0 : EIO;

            if (0 == g_providerStatus)
            {
                g_api.OPENSSL_load_builtin_modules();

                if (NULL != g_api.ENGINE_load_builtin_engines)
                {
                    g_api.ENGINE_load_builtin_engines();
                }

                // Unlike OPENSSL_config(), this API exposes configuration failures.
                configuration = getenv("OPENSSL_CONF");

                if (NULL == configuration)
                {
                    configurationFlags |= TlsConfigIgnoreMissingFile;
                }

                g_api.ERR_clear_error();
                operation = "CONF_modules_load_file";
                g_providerStatus = (0 < g_api.CONF_modules_load_file(configuration, NULL, configurationFlags)) ? 0 : EIO;
            }
        }

        if (0 != g_providerStatus)
        {
            OsConfigLogError(log, "LoadProvider: %s failed with %d (%s); provider %s", operation, g_providerStatus, strerror(g_providerStatus), providers[i].name);
        }

        return g_providerStatus;
    }

    if (NULL != missingSymbol)
    {
        OsConfigLogError(log, "LoadProvider: dlsym(%s) failed with %d (%s); last rejected provider %s, no usable OpenSSL runtime",
            missingSymbol, apiStatus, strerror(apiStatus), rejectedProvider);
    }
    else if (NULL != rejectedProvider)
    {
        OsConfigLogError(log, "LoadProvider: OpenSSL version compatibility check failed with %d (%s); last rejected provider %s, no usable OpenSSL runtime",
            apiStatus, strerror(apiStatus), rejectedProvider);
    }
    else
    {
        OsConfigLogError(log, "LoadProvider: supported OpenSSL runtime discovery failed with %d (%s); no usable OpenSSL 3, 1.1 or 1.0.2 runtime",
            g_providerStatus, strerror(g_providerStatus));
    }

    return g_providerStatus;
}

static int WorkerSignals(OsConfigLogHandle log)
{
    struct sigaction action = {0};
    int status = 0;

    if (0 != sigaction(SIGPIPE, NULL, &action))
    {
        status = (0 != errno) ? errno : EIO;
        OsConfigLogError(log, "WorkerSignals: sigaction(SIGPIPE) failed with %d (%s)", status, strerror(status));
    }
    else
    {
        status = (SIG_IGN == action.sa_handler) ? 0 : ENOTSUP;
        if (0 != status)
        {
            OsConfigLogError(log, "WorkerSignals: SIGPIPE disposition check failed with %d (%s)", status, strerror(status));
        }
    }

    return status;
}

static void Disconnect(TelemetryTls* tls)
{
    if (NULL != tls->connection)
    {
        g_api.SSL_free(tls->connection);
        tls->connection = NULL;
    }

    tls->connected = false;
    tls->descriptor = -1;
}

static int Failure(TelemetryTls* tls, const char* function, const char* operation, int status, OsConfigLogHandle log)
{
    unsigned long providerError = (g_attempted && (NULL != g_api.ERR_get_error)) ? g_api.ERR_get_error() : 0;
    char providerText[256] = "provider error text unavailable";

    if ((0 != providerError) && (NULL != g_api.ERR_error_string_n))
    {
        g_api.ERR_error_string_n(providerError, providerText, sizeof(providerText));
    }

    if (0 != providerError)
    {
        OsConfigLogError(log, "%s: %s failed with %d (%s); OpenSSL error 0x%lx (%s)", function, operation, status, strerror(status), providerError, providerText);
    }
    else
    {
        OsConfigLogError(log, "%s: %s failed with %d (%s)", function, operation, status, strerror(status));
    }

    if (NULL != tls)
    {
        Disconnect(tls);
    }

    return status;
}

static int Wait(TelemetryTls* tls, int sslError, int systemError, int64_t deadline, OsConfigLogHandle log)
{
    int remaining = 0;
    int status = 0;
    struct pollfd item = {0};
    int ready = 0;
    const char* operation = "SSL_get_error";

    if ((TlsWantRead != sslError) && (TlsWantWrite != sslError))
    {
        if (0 != g_api.SSL_get_verify_result(tls->connection))
        {
            operation = "SSL_get_verify_result";
            status = EACCES;
        }
        else if ((TlsSyscallError == sslError) && (0 != systemError))
        {
            status = systemError;
        }
        else
        {
            status = EPROTO;
        }
    }
    else
    {
        while ((0 == status) && (ready <= 0))
        {
            remaining = 0;
            operation = "TelemetryDeadlineRemaining";

            if (0 == (status = TelemetryDeadlineRemaining(deadline, &remaining)))
            {
                item = (struct pollfd){tls->descriptor, (short)((TlsWantRead == sslError) ? POLLIN : POLLOUT), 0};
                operation = "poll";
                ready = poll(&item, 1, remaining);

                if (ready > 0)
                {
                    status = (0 != (item.revents & POLLNVAL)) ? EBADF : 0;
                }
                else if ((ready < 0) && (EINTR != errno))
                {
                    status = (0 != errno) ? errno : EIO;
                }
            }
        }
    }

    if (0 != status)
    {
        OsConfigLogError(log, "Wait: %s failed with %d (%s); SSL_get_error returned %d", operation, status, strerror(status), sslError);
    }

    return status;
}

int TelemetryTlsCreate(TelemetryTls** tls, int64_t deadline, OsConfigLogHandle log)
{
    int remaining = 0;
    int status = 0;
    TelemetryTls* created = NULL;
    static const unsigned char http11[] = {8, 'h', 't', 't', 'p', '/', '1', '.', '1'};
    long minimum = 0;
    const char* operation = "output validation";

    if (NULL == tls)
    {
        status = EINVAL;
    }
    else if (NULL != *tls)
    {
        status = EALREADY;
    }
    else if (0 != (status = WorkerSignals(log)))
    {
        operation = "WorkerSignals";
    }
    else if (0 != (status = TelemetryDeadlineRemaining(deadline, &remaining)))
    {
        operation = "TelemetryDeadlineRemaining";
    }
    else
    {
        status = LoadProvider(log);

        // Provider initialization/configuration failures never trigger fallback.
        if (0 != status)
        {
            return Failure(NULL, __func__, "LoadProvider", status, log);
        }

        g_api.ERR_clear_error();
        created = calloc(1, sizeof(*created));

        if (NULL == created)
        {
            operation = "calloc";
            status = ENOMEM;
        }
        else
        {
            created->descriptor = -1;
            created->context = g_api.SSL_CTX_new(g_api.TLS_client_method());

            if (NULL == created->context)
            {
                operation = "SSL_CTX_new";
                status = EIO;
            }
            else
            {
                // Legacy uses TLSv1_2_client_method; newer providers retain higher OS minimums.
                if (NULL != g_api.OPENSSL_init_ssl)
                {
                    minimum = g_api.SSL_CTX_ctrl(created->context, TlsGetMinimumVersion, 0, NULL);

                    if ((minimum < TlsVersion12) && (1 != g_api.SSL_CTX_ctrl(created->context, TlsSetMinimumVersion, TlsVersion12, NULL)))
                    {
                        operation = "SSL_CTX_ctrl(TlsSetMinimumVersion)";
                        status = EIO;
                    }
                }

                g_api.SSL_CTX_set_verify(created->context, TlsVerifyPeer, NULL);

                if (0 != g_api.SSL_CTX_set_alpn_protos(created->context, http11, sizeof(http11)))
                {
                    operation = "SSL_CTX_set_alpn_protos";
                    status = EIO;
                }

                if (NULL != g_api.SetOptions3)
                {
                    if (0 == (g_api.SetOptions3(created->context, TlsNoCompression) & TlsNoCompression))
                    {
                        operation = "SSL_CTX_set_options";
                        status = EIO;
                    }
                    else if (0 != (g_api.ClearOptions3(created->context, TlsIgnoreUnexpectedEof) & TlsIgnoreUnexpectedEof))
                    {
                        operation = "SSL_CTX_clear_options";
                        status = EIO;
                    }
                }
                else if (NULL != g_api.SetOptions11)
                {
                    if (0 == (g_api.SetOptions11(created->context, TlsNoCompression) & TlsNoCompression))
                    {
                        operation = "SSL_CTX_set_options";
                        status = EIO;
                    }
                }
                else if (0 == (g_api.SSL_CTX_ctrl(created->context, TlsSetOptions, TlsNoCompression, NULL) & TlsNoCompression))
                {
                    operation = "SSL_CTX_ctrl(TlsSetOptions)";
                    status = EIO;
                }

                if ((0 == status) && (1 != g_api.SSL_CTX_set_default_verify_paths(created->context)))
                {
                    operation = "SSL_CTX_set_default_verify_paths";
                    status = EIO;
                }

                if (0 == status)
                {
                    operation = "TelemetryDeadlineRemaining";
                    status = TelemetryDeadlineRemaining(deadline, &remaining);
                }
            }
        }
    }

    if (0 != status)
    {
        Failure(NULL, __func__, operation, status, log);
        TelemetryTlsDestroy(&created, log);
    }
    else
    {
        *tls = created;
    }

    return status;
}

int TelemetryTlsHandshake(TelemetryTls* tls, int descriptor, const char* peer, int64_t deadline, OsConfigLogHandle log)
{
    int status = 0;
    int remaining = 0;
    unsigned char address[16] = {0};
    bool ip = false;
    const char* c = NULL;
    int flags = 0;
    VerifyParameters* parameters = NULL;
    int done = 0;
    int systemError = 0;
    int sslError = 0;
    Certificate* certificate = NULL;
    bool verified = false;
    const unsigned char* protocol = NULL;
    unsigned int protocolSize = 0;
    unsigned int hostFlags = TlsNoPartialWildcards;
    const char* operation = "argument validation";

    if ((NULL == tls) || (NULL == peer) || ('\0' == *peer) || (strnlen(peer, 254) > 253) || (descriptor < 0))
    {
        status = EINVAL;
    }
    else if (NULL != tls->connection)
    {
        // Do not invalidate an existing session on an accidental second attach.
        return Failure(NULL, __func__, "connection state validation", EALREADY, log);
    }
    else
    {
        ip = (1 == inet_pton(AF_INET, peer, address)) || (1 == inet_pton(AF_INET6, peer, address));

        for (c = peer; ('\0' != *c) && (0 == status) && !ip; ++c)
        {
            if (!(((*c >= 'a') && (*c <= 'z')) || ((*c >= 'A') && (*c <= 'Z')) || ((*c >= '0') && (*c <= '9')) || ('-' == *c) || ('.' == *c) || ('_' == *c)))
            {
                status = EINVAL;
            }
        }

        flags = fcntl(descriptor, F_GETFL);

        if (flags < 0)
        {
            operation = "fcntl(F_GETFL)";
            status = (0 != errno) ? errno : EIO;
        }
        else if (!(flags & O_NONBLOCK))
        {
            operation = "nonblocking descriptor validation";
            status = EINVAL;
        }

        if (0 == status)
        {
            operation = "WorkerSignals";
            status = WorkerSignals(log);
        }

        if (0 == status)
        {
            operation = "TelemetryDeadlineRemaining";
            status = TelemetryDeadlineRemaining(deadline, &remaining);
        }
    }

    if (0 != status)
    {
        return Failure(NULL, __func__, operation, status, log);
    }

    g_api.ERR_clear_error();
    tls->descriptor = descriptor;
    tls->connection = g_api.SSL_new(tls->context);

    if (NULL == tls->connection)
    {
        return Failure(tls, __func__, "SSL_new", EIO, log);
    }

    parameters = g_api.SSL_get0_param(tls->connection);

    if ((NULL == parameters) || (1 != g_api.SSL_set_fd(tls->connection, descriptor)))
    {
        return Failure(tls, __func__, (NULL == parameters) ? "SSL_get0_param" : "SSL_set_fd", EIO, log);
    }

    if (NULL != g_api.X509_VERIFY_PARAM_get_hostflags)
    {
        hostFlags |= g_api.X509_VERIFY_PARAM_get_hostflags(parameters);
    }

    g_api.X509_VERIFY_PARAM_set_hostflags(parameters, hostFlags);

    if (ip)
    {
        if (1 != g_api.X509_VERIFY_PARAM_set1_ip_asc(parameters, peer))
        {
            operation = "X509_VERIFY_PARAM_set1_ip_asc";
            status = EINVAL;
        }
    }
    else if (1 != g_api.X509_VERIFY_PARAM_set1_host(parameters, peer, 0))
    {
        operation = "X509_VERIFY_PARAM_set1_host";
        status = EIO;
    }
    else if (1 != g_api.SSL_ctrl(tls->connection, TlsSetServerName, 0, (void*)peer))
    {
        operation = "SSL_ctrl(TlsSetServerName)";
        status = EIO;
    }

    if (0 != status)
    {
        return Failure(tls, __func__, operation, status, log);
    }

    for (;;)
    {
        if (0 != (status = TelemetryDeadlineRemaining(deadline, &remaining)))
        {
            return Failure(tls, __func__, "TelemetryDeadlineRemaining", status, log);
        }

        g_api.ERR_clear_error();
        errno = 0;
        done = g_api.SSL_connect(tls->connection);
        systemError = errno;

        if (1 == done)
        {
            break;
        }

        sslError = g_api.SSL_get_error(tls->connection, done);

        if (0 != (status = Wait(tls, sslError, systemError, deadline, log)))
        {
            return Failure(tls, __func__, "SSL_connect", status, log);
        }
    }

    certificate = g_api.GetPeerCertificate(tls->connection);
    operation = (NULL == certificate) ? "SSL_get_peer_certificate" : "SSL_get_verify_result";
    verified = (NULL != certificate) && (0 == g_api.SSL_get_verify_result(tls->connection));

    if (NULL != certificate)
    {
        g_api.X509_free(certificate);
    }

    if (!verified)
    {
        return Failure(tls, __func__, operation, EACCES, log);
    }

    g_api.SSL_get0_alpn_selected(tls->connection, &protocol, &protocolSize);

    if ((0 != protocolSize) && ((8 != protocolSize) || (NULL == protocol) || (0 != memcmp(protocol, "http/1.1", 8))))
    {
        return Failure(tls, __func__, "SSL_get0_alpn_selected protocol validation", ENOTSUP, log);
    }

    if (0 != (status = TelemetryDeadlineRemaining(deadline, &remaining)))
    {
        return Failure(tls, __func__, "TelemetryDeadlineRemaining", status, log);
    }

    tls->connected = true;

    return 0;
}

int TelemetryTlsWrite(TelemetryTls* tls, const void* bytes, size_t size, int64_t deadline, OsConfigLogHandle log)
{
    size_t offset = 0;
    int done = 0;
    int systemError = 0;
    int sslError = 0;
    int remaining = 0;
    int status = 0;

    if ((NULL == tls) || !tls->connected || ((NULL == bytes) && (0 != size)) || (size > INT_MAX))
    {
        return Failure(NULL, __func__, "argument validation", EINVAL, log);
    }

    while (offset < size)
    {
        remaining = 0;

        if (0 != (status = TelemetryDeadlineRemaining(deadline, &remaining)))
        {
            return Failure(tls, __func__, "TelemetryDeadlineRemaining", status, log);
        }

        g_api.ERR_clear_error();
        errno = 0;
        done = g_api.SSL_write(tls->connection, (const unsigned char*)bytes + offset, (int)(size - offset));
        systemError = errno;

        if (done > 0)
        {
            offset += (size_t)done;
            continue;
        }

        sslError = g_api.SSL_get_error(tls->connection, done);

        if (0 != (status = Wait(tls, sslError, systemError, deadline, log)))
        {
            return Failure(tls, __func__, "SSL_write", status, log);
        }
    }

    remaining = 0;
    status = TelemetryDeadlineRemaining(deadline, &remaining);

    return (0 != status) ? Failure(tls, __func__, "TelemetryDeadlineRemaining", status, log) : 0;
}

int TelemetryTlsRead(TelemetryTls* tls, void* bytes, size_t capacity, size_t* size, bool* endOfStream, int64_t deadline, OsConfigLogHandle log)
{
    int remaining = 0;
    int status = 0;
    int done = 0;
    int systemError = 0;
    int sslError = 0;

    if (NULL != size)
    {
        *size = 0;
    }

    if (NULL != endOfStream)
    {
        *endOfStream = false;
    }

    if ((NULL == tls) || !tls->connected || (NULL == bytes) || (0 == capacity) || (capacity > INT_MAX) || (NULL == size) || (NULL == endOfStream))
    {
        return Failure(NULL, __func__, "argument validation", EINVAL, log);
    }

    for (;;)
    {
        remaining = 0;

        if (0 != (status = TelemetryDeadlineRemaining(deadline, &remaining)))
        {
            return Failure(tls, __func__, "TelemetryDeadlineRemaining", status, log);
        }

        g_api.ERR_clear_error();
        errno = 0;
        done = g_api.SSL_read(tls->connection, bytes, (int)capacity);
        systemError = errno;
        sslError = (done > 0) ? 0 : g_api.SSL_get_error(tls->connection, done);

        if ((done > 0) || (TlsClosed == sslError))
        {
            if (0 != (status = TelemetryDeadlineRemaining(deadline, &remaining)))
            {
                return Failure(tls, __func__, "TelemetryDeadlineRemaining", status, log);
            }

            *size = (done > 0) ? (size_t)done : 0;
            *endOfStream = (TlsClosed == sslError);

            if (*endOfStream)
            {
                Disconnect(tls);
            }

            return 0;
        }

        if (0 != (status = Wait(tls, sslError, systemError, deadline, log)))
        {
            return Failure(tls, __func__, "SSL_read", status, log);
        }
    }
}

void TelemetryTlsDestroy(TelemetryTls** tls, OsConfigLogHandle log)
{
    (void)log;
    if ((NULL == tls) || (NULL == *tls))
    {
        return;
    }

    Disconnect(*tls);

    if (NULL != (*tls)->context)
    {
        g_api.SSL_CTX_free((*tls)->context);
    }

    free(*tls);
    *tls = NULL;
}
