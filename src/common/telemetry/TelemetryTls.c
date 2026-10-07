// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "TelemetryTls.h"
#include "TelemetryDeadline.h"

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

static int LoadApi(void* library, TlsApi* api, unsigned long minimumVersion,
    unsigned long maximumVersion, OsConfigLogHandle log)
{
    void* symbol = NULL;
    void* peer = NULL;
    unsigned long version = 0;

#define LOAD_TLS_NAMED(member, name) do \
{ \
    symbol = dlsym(library, name); \
    \
    if (NULL == symbol) \
    { \
        OsConfigLogInfo(log, "TelemetryTls: Provider lacks %s", name); \
        return ENOSYS; \
    } \
    \
    _Static_assert(sizeof(symbol) == sizeof(api->member), "Unsupported POSIX function pointer ABI"); \
    memcpy(&api->member, &symbol, sizeof(symbol)); \
} while (0)
#define LOAD_TLS_SYMBOL(member) LOAD_TLS_NAMED(member, #member)
    LOAD_TLS_NAMED(OpenSSL_version_num, minimumVersion < 0x10100000UL ? "SSLeay" : "OpenSSL_version_num");
    version = api->OpenSSL_version_num();

    if ((version < minimumVersion) || (version >= maximumVersion))
    {
        OsConfigLogInfo(log, "TelemetryTls: Unsupported provider version 0x%lx", version);
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

        if (NULL == symbol)
        {
            OsConfigLogInfo(log, "TelemetryTls: Legacy provider has no built-in engine loader");
        }
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
        OsConfigLogInfo(log, "TelemetryTls: Provider lacks peer-certificate access");
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
    unsigned long version = 0;
    const char* configuration = NULL;
    unsigned long configurationFlags = TlsConfigDefaultSection;

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
            OsConfigLogInfo(log, "TelemetryTls: OS provider %s unavailable", providers[i].name);
            continue;
        }

        if (0 != LoadApi(g_libraries[i], &api, providers[i].minimum, providers[i].maximum, log))
        {
            OsConfigLogInfo(log, "TelemetryTls: OS provider %s has no supported API/ABI", providers[i].name);
            continue;
        }

        version = api.OpenSSL_version_num();
        g_api = api;

        if (NULL != g_api.OPENSSL_init_ssl)
        {
            g_providerStatus = (1 == g_api.OPENSSL_init_ssl(TlsLoadConfiguration, NULL)) ? 0 : EIO;
        }
        else
        {
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
                g_providerStatus = (0 < g_api.CONF_modules_load_file(
                    configuration, NULL, configurationFlags)) ? 0 : EIO;
            }
        }

        OsConfigLogInfo(log, "TelemetryTls: Selected %s (version=0x%lx, status=%d)",
            providers[i].name, version, g_providerStatus);

        return g_providerStatus;
    }

    OsConfigLogError(log, "TelemetryTls: No usable system OpenSSL 3, 1.1 or 1.0.2 runtime; telemetry cannot be sent");
    return g_providerStatus;
}

static int WorkerSignals(void)
{
    struct sigaction action = {0};
    int status = 0;

    if (0 != sigaction(SIGPIPE, NULL, &action))
    {
        status = errno ? errno : EIO;
    }
    else
    {
        status = (SIG_IGN == action.sa_handler) ? 0 : ENOTSUP;
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

static int Failure(TelemetryTls* tls, const char* operation, int status, OsConfigLogHandle log)
{
    unsigned long providerError = ((g_attempted) && (g_api.ERR_get_error)) ? g_api.ERR_get_error() : 0;

    OsConfigLogError(log, "TelemetryTls: %s failed (status=%d, provider=0x%lx)",
        operation, status, providerError);

    if (NULL != tls)
    {
        Disconnect(tls);
    }

    return status;
}

static int Wait(TelemetryTls* tls, int sslError, int systemError, int64_t deadline)
{
    int remaining = 0;
    int status = 0;
    struct pollfd item = {0};
    int ready = 0;

    if ((TlsWantRead != sslError) && (TlsWantWrite != sslError))
    {
        if (0 != g_api.SSL_get_verify_result(tls->connection))
        {
            status = EACCES;
        }
        else if ((TlsSyscallError == sslError) && (systemError))
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

            if (0 == (status = TelemetryDeadlineRemaining(deadline, &remaining)))
            {
                item = (struct pollfd){tls->descriptor, (short)((TlsWantRead == sslError) ? POLLIN : POLLOUT), 0};
                ready = poll(&item, 1, remaining);

                if (ready > 0)
                {
                    status = item.revents & POLLNVAL ? EBADF : 0;
                }
                else if ((ready < 0) && (EINTR != errno))
                {
                    status = errno ? errno : EIO;
                }
            }
        }
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

    if (NULL == tls)
    {
        status = EINVAL;
    }
    else if (NULL != *tls)
    {
        status = EALREADY;
    }
    else if ((0 == (status = WorkerSignals())) &&
        (0 == (status = TelemetryDeadlineRemaining(deadline, &remaining))))
    {
        status = LoadProvider(log);

        // Provider initialization/configuration failures never trigger fallback.
        if (status)
        {
            return Failure(NULL, "OS provider initialization", status, log);
        }

        g_api.ERR_clear_error();
        created = calloc(1, sizeof(*created));

        if (NULL == created)
        {
            status = ENOMEM;
        }
        else
        {
            created->descriptor = -1;
            created->context = g_api.SSL_CTX_new(g_api.TLS_client_method());

            if (NULL == created->context)
            {
                status = EIO;
            }
            else
            {
                // Legacy uses TLSv1_2_client_method; newer providers retain higher OS minimums.
                if (NULL != g_api.OPENSSL_init_ssl)
                {
                    minimum = g_api.SSL_CTX_ctrl(created->context, TlsGetMinimumVersion, 0, NULL);

                    if ((minimum < TlsVersion12) &&
                        (1 != g_api.SSL_CTX_ctrl(created->context, TlsSetMinimumVersion, TlsVersion12, NULL)))
                    {
                        status = EIO;
                    }
                }

                g_api.SSL_CTX_set_verify(created->context, TlsVerifyPeer, NULL);

                if (0 != g_api.SSL_CTX_set_alpn_protos(created->context, http11, sizeof(http11)))
                {
                    status = EIO;
                }

                if (NULL != g_api.SetOptions3)
                {
                    if ((!(g_api.SetOptions3(created->context, TlsNoCompression) & TlsNoCompression)) ||
                        (g_api.ClearOptions3(created->context, TlsIgnoreUnexpectedEof) & TlsIgnoreUnexpectedEof))
                    {
                        status = EIO;
                    }
                }
                else if (NULL != g_api.SetOptions11)
                {
                    if (!(g_api.SetOptions11(created->context, TlsNoCompression) & TlsNoCompression))
                    {
                        status = EIO;
                    }
                }
                else if (!(g_api.SSL_CTX_ctrl(created->context, TlsSetOptions, TlsNoCompression, NULL) & TlsNoCompression))
                {
                    status = EIO;
                }

                if ((0 == status) && (1 != g_api.SSL_CTX_set_default_verify_paths(created->context)))
                {
                    status = EIO;
                }

                if (0 == status)
                {
                    status = TelemetryDeadlineRemaining(deadline, &remaining);
                }
            }
        }
    }

    if (0 != status)
    {
        Failure(NULL, "initialization", status, log);
        TelemetryTlsDestroy(&created, log);
    }
    else
    {
        *tls = created;
        OsConfigLogInfo(log, "TelemetryTls: Initialized OS trust and peer verification");
    }

    return status;
}

int TelemetryTlsHandshake(TelemetryTls* tls, int descriptor, const char* peer,
    int64_t deadline, OsConfigLogHandle log)
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

    if ((NULL == tls) || (NULL == peer) || ('\0' == *peer) ||
        (strnlen(peer, 254) > 253) || (descriptor < 0))
    {
        status = EINVAL;
    }
    else if (NULL != tls->connection)
    {
        // Do not invalidate an existing session on an accidental second attach.
        return Failure(NULL, "duplicate handshake", EALREADY, log);
    }
    else
    {
        ip = (1 == inet_pton(AF_INET, peer, address)) || (1 == inet_pton(AF_INET6, peer, address));

        for (c = peer; (*c) && (!status) && (!ip); ++c)
        {
            if (!(((*c >= 'a') && (*c <= 'z')) || ((*c >= 'A') && (*c <= 'Z')) ||
                ((*c >= '0') && (*c <= '9')) || ('-' == *c) || ('.' == *c) || ('_' == *c)))
            {
                status = EINVAL;
            }
        }

        flags = fcntl(descriptor, F_GETFL);

        if (flags < 0)
        {
            status = errno ? errno : EIO;
        }
        else if (!(flags & O_NONBLOCK))
        {
            status = EINVAL;
        }

        if (0 == status)
        {
            status = WorkerSignals();
        }

        if (0 == status)
        {
            status = TelemetryDeadlineRemaining(deadline, &remaining);
        }
    }

    if (0 != status)
    {
        return Failure(NULL, "handshake arguments", status, log);
    }

    g_api.ERR_clear_error();
    tls->descriptor = descriptor;
    tls->connection = g_api.SSL_new(tls->context);

    if (NULL == tls->connection)
    {
        return Failure(tls, "session allocation", EIO, log);
    }

    parameters = g_api.SSL_get0_param(tls->connection);

    if ((NULL == parameters) || (1 != g_api.SSL_set_fd(tls->connection, descriptor)))
    {
        return Failure(tls, "socket setup", EIO, log);
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
            status = EINVAL;
        }
    }
    else if ((1 != g_api.X509_VERIFY_PARAM_set1_host(parameters, peer, 0)) ||
        (1 != g_api.SSL_ctrl(tls->connection, TlsSetServerName, 0, (void*)peer)))
    {
        status = EIO;
    }

    if (0 != status)
    {
        return Failure(tls, "peer identity setup", status, log);
    }

    for (;;)
    {
        if (0 != (status = TelemetryDeadlineRemaining(deadline, &remaining)))
        {
            return Failure(tls, "handshake deadline", status, log);
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

        if (0 != (status = Wait(tls, sslError, systemError, deadline)))
        {
            return Failure(tls, "handshake", status, log);
        }
    }

    certificate = g_api.GetPeerCertificate(tls->connection);
    verified = (NULL != certificate) && (0 == g_api.SSL_get_verify_result(tls->connection));

    if (NULL != certificate)
    {
        g_api.X509_free(certificate);
    }

    if (!verified)
    {
        return Failure(tls, "certificate verification", EACCES, log);
    }

    g_api.SSL_get0_alpn_selected(tls->connection, &protocol, &protocolSize);

    if ((protocolSize) && ((8 != protocolSize) || (NULL == protocol) || (memcmp(protocol, "http/1.1", 8))))
    {
        return Failure(tls, "negotiated HTTP protocol", ENOTSUP, log);
    }

    if (0 != (status = TelemetryDeadlineRemaining(deadline, &remaining)))
    {
        return Failure(tls, "handshake deadline", status, log);
    }

    tls->connected = true;
    OsConfigLogInfo(log, "TelemetryTls: Verified secure connection established");

    return 0;
}

int TelemetryTlsWrite(TelemetryTls* tls, const void* bytes, size_t size,
    int64_t deadline, OsConfigLogHandle log)
{
    size_t offset = 0;
    int done = 0;
    int systemError = 0;
    int sslError = 0;
    int remaining = 0;
    int status = 0;

    if ((NULL == tls) || (!tls->connected) || ((NULL == bytes) && (size)) || (size > INT_MAX))
    {
        return Failure(NULL, "write arguments", EINVAL, log);
    }

    while (offset < size)
    {
        remaining = 0;

        if (0 != (status = TelemetryDeadlineRemaining(deadline, &remaining)))
        {
            return Failure(tls, "write deadline", status, log);
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

        if (0 != (status = Wait(tls, sslError, systemError, deadline)))
        {
            return Failure(tls, "write", status, log);
        }
    }

    remaining = 0;
    status = TelemetryDeadlineRemaining(deadline, &remaining);

    return status ? Failure(tls, "write deadline", status, log) : 0;
}

int TelemetryTlsRead(TelemetryTls* tls, void* bytes, size_t capacity, size_t* size,
    bool* endOfStream, int64_t deadline, OsConfigLogHandle log)
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

    if ((NULL == tls) || (!tls->connected) || (NULL == bytes) || (!capacity) ||
        (capacity > INT_MAX) || (NULL == size) || (NULL == endOfStream))
    {
        return Failure(NULL, "read arguments", EINVAL, log);
    }

    for (;;)
    {
        remaining = 0;

        if (0 != (status = TelemetryDeadlineRemaining(deadline, &remaining)))
        {
            return Failure(tls, "read deadline", status, log);
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
                return Failure(tls, "read deadline", status, log);
            }

            *size = (done > 0) ? (size_t)done : 0;
            *endOfStream = (TlsClosed == sslError);

            if (*endOfStream)
            {
                Disconnect(tls);
            }

            return 0;
        }

        if (0 != (status = Wait(tls, sslError, systemError, deadline)))
        {
            return Failure(tls, "read", status, log);
        }
    }
}

void TelemetryTlsDestroy(TelemetryTls** tls, OsConfigLogHandle log)
{
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
    OsConfigLogInfo(log, "TelemetryTls: Released TLS context");
}
