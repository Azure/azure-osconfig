// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "TelemetryTls.h"
#include "TelemetryDeadline.h"
#ifdef OSCONFIG_TELEMETRY_MINTLS
#include <MinTls.h>
#endif

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
    TlsSetMinimumVersion = 123,
    TlsGetMinimumVersion = 130,
    TlsVersion12 = 0x0303,
    TlsVerifyPeer = 1,
    TlsNoPartialWildcards = 4,
    TlsLoadConfiguration = 0x40,
    TlsIgnoreUnexpectedEof = 0x80,
    TlsNoCompression = 0x20000
};

typedef struct TlsApi
{
    int (*OPENSSL_init_ssl)(uint64_t, const InitSettings*);
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
    int (*SSL_set1_host)(Ssl*, const char*);
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
#ifdef OSCONFIG_TELEMETRY_MINTLS
    MinTls* fallback;
#endif
    SslContext* context;
    Ssl* connection;
    int descriptor;
    bool connected;
};

// Private to the exec'd worker. Never unload provider code underneath its
// registered runtime callbacks; the owned worker's exit reclaims these maps.
static TlsApi g_api;
static void* g_libraries[2];
static bool g_attempted = false;
static int g_providerStatus = ENOTSUP;

static int LoadApi(void* library, TlsApi* api, OsConfigLogHandle log)
{
#define LOAD_TLS_NAMED(member, name) do { \
    void* symbol = dlsym(library, name); \
    if (NULL == symbol) { \
        OsConfigLogInfo(log, "TelemetryTls: Provider lacks %s", name); \
        return ENOSYS; \
    } \
    _Static_assert(sizeof(symbol) == sizeof(api->member), "Unsupported POSIX function pointer ABI"); \
    memcpy(&api->member, &symbol, sizeof(symbol)); \
} while (0)
#define LOAD_TLS_SYMBOL(member) LOAD_TLS_NAMED(member, #member)
    LOAD_TLS_SYMBOL(OPENSSL_init_ssl);
    LOAD_TLS_SYMBOL(OpenSSL_version_num);
    LOAD_TLS_SYMBOL(TLS_client_method);
    LOAD_TLS_SYMBOL(SSL_CTX_new);
    LOAD_TLS_SYMBOL(SSL_CTX_free);
    LOAD_TLS_SYMBOL(SSL_CTX_ctrl);
    LOAD_TLS_SYMBOL(SSL_CTX_set_verify);
    LOAD_TLS_SYMBOL(SSL_CTX_set_default_verify_paths);
    LOAD_TLS_SYMBOL(SSL_CTX_set_alpn_protos);
    LOAD_TLS_SYMBOL(SSL_new);
    LOAD_TLS_SYMBOL(SSL_free);
    LOAD_TLS_SYMBOL(SSL_set_fd);
    LOAD_TLS_SYMBOL(SSL_set1_host);
    LOAD_TLS_SYMBOL(SSL_get0_param);
    LOAD_TLS_SYMBOL(X509_VERIFY_PARAM_get_hostflags);
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
    if (api->OpenSSL_version_num() >= 0x30000000UL)
    {
        LOAD_TLS_NAMED(SetOptions3, "SSL_CTX_set_options");
        LOAD_TLS_NAMED(ClearOptions3, "SSL_CTX_clear_options");
    }
    else
    {
        LOAD_TLS_NAMED(SetOptions11, "SSL_CTX_set_options");
    }
#undef LOAD_TLS_SYMBOL
#undef LOAD_TLS_NAMED
    void* peer = dlsym(library, "SSL_get1_peer_certificate");
    if (NULL == peer) peer = dlsym(library, "SSL_get_peer_certificate");
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
#ifdef OSCONFIG_TELEMETRY_FORCE_MINTLS
    OsConfigLogInfo(log, "TelemetryTls: Test build forces mintls; OS provider discovery disabled");
    return ENOTSUP;
#endif
    if (g_attempted) return g_providerStatus;
    g_attempted = true;
    static const char* names[] = {"libssl.so.3", "libssl.so.1.1"};
    for (size_t i = 0; i < ARRAY_SIZE(names); ++i)
    {
        TlsApi api = {0};
        g_libraries[i] = dlopen(names[i], RTLD_NOW | RTLD_LOCAL);
        if (NULL == g_libraries[i])
        {
            OsConfigLogInfo(log, "TelemetryTls: OS provider %s unavailable", names[i]);
            continue;
        }
        if (0 != LoadApi(g_libraries[i], &api, log)) continue;
        unsigned long version = api.OpenSSL_version_num();
        bool supported = (i == 0) ?
            (version >= 0x30000000UL && version < 0x40000000UL) :
            (version >= 0x10100000UL && version < 0x10200000UL);
        if (!supported)
        {
            OsConfigLogInfo(log, "TelemetryTls: Unsupported provider ABI in %s", names[i]);
            continue;
        }
        g_api = api;
        // Respect the OS OpenSSL configuration, including provider/security policy.
        g_providerStatus = (1 == g_api.OPENSSL_init_ssl(TlsLoadConfiguration, NULL)) ? 0 : EIO;
        OsConfigLogInfo(log, "TelemetryTls: Selected %s (version=0x%lx, status=%d)",
            names[i], version, g_providerStatus);
        return g_providerStatus;
    }
    return g_providerStatus;
}

static int WorkerSignals(void)
{
    struct sigaction action = {0};
    if (0 != sigaction(SIGPIPE, NULL, &action)) return errno ? errno : EIO;
    return (SIG_IGN == action.sa_handler) ? 0 : ENOTSUP;
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
    unsigned long providerError = g_attempted && g_api.ERR_get_error ? g_api.ERR_get_error() : 0;
    OsConfigLogError(log, "TelemetryTls: %s failed (status=%d, provider=0x%lx)",
        operation, status, providerError);
    if (NULL != tls) Disconnect(tls);
    return status;
}

static int Wait(TelemetryTls* tls, int sslError, int systemError, int64_t deadline)
{
    if ((TlsWantRead != sslError) && (TlsWantWrite != sslError))
    {
        if (0 != g_api.SSL_get_verify_result(tls->connection)) return EACCES;
        if ((TlsSyscallError == sslError) && systemError) return systemError;
        return EPROTO;
    }
    for (;;)
    {
        int remaining = 0;
        int status = TelemetryDeadlineRemaining(deadline, &remaining);
        if (0 != status) return status;
        struct pollfd item = {tls->descriptor, (short)((TlsWantRead == sslError) ? POLLIN : POLLOUT), 0};
        int ready = poll(&item, 1, remaining);
        if (ready > 0)
        {
            if (item.revents & POLLNVAL) return EBADF;
            return 0;
        }
        if ((ready < 0) && (EINTR != errno)) return errno ? errno : EIO;
    }
}

int TelemetryTlsCreate(TelemetryTls** tls, int64_t deadline, OsConfigLogHandle log)
{
    int remaining = 0;
    int status = 0;
    TelemetryTls* created = NULL;
    if (NULL == tls) status = EINVAL;
    else if (NULL != *tls) status = EALREADY;
    else if ((0 == (status = WorkerSignals())) &&
        (0 == (status = TelemetryDeadlineRemaining(deadline, &remaining))))
    {
        status = LoadProvider(log);
#ifdef OSCONFIG_TELEMETRY_MINTLS
        if (status == ENOTSUP)
        {
            created = calloc(1, sizeof(*created));
            if (!created) return Failure(NULL, "fallback allocation", ENOMEM, log);
            created->descriptor = -1;
            status = MinTlsCreate(&created->fallback, NULL, deadline, log);
            if (status)
            {
                TelemetryTlsDestroy(&created, log);
                return Failure(NULL, "fallback initialization", status, log);
            }
            *tls = created;
            OsConfigLogInfo(log, "TelemetryTls: Selected in-tree mintls fallback");
            return 0;
        }
#endif
        // Provider initialization/configuration failures never trigger fallback.
        if (status) return Failure(NULL, "OS provider initialization", status, log);
        g_api.ERR_clear_error();
        created = calloc(1, sizeof(*created));
        if (NULL == created) status = ENOMEM;
        else
        {
            created->descriptor = -1;
            created->context = g_api.SSL_CTX_new(g_api.TLS_client_method());
            if (NULL == created->context) status = EIO;
            else
            {
                static const unsigned char http11[] = {8, 'h', 't', 't', 'p', '/', '1', '.', '1'};
                long minimum = g_api.SSL_CTX_ctrl(created->context, TlsGetMinimumVersion, 0, NULL);
                if ((minimum < TlsVersion12) &&
                    (1 != g_api.SSL_CTX_ctrl(created->context, TlsSetMinimumVersion, TlsVersion12, NULL)))
                {
                    status = EIO;
                }
                g_api.SSL_CTX_set_verify(created->context, TlsVerifyPeer, NULL);
                if (0 != g_api.SSL_CTX_set_alpn_protos(created->context, http11, sizeof(http11))) status = EIO;
                if (NULL != g_api.SetOptions3)
                {
                    if (!(g_api.SetOptions3(created->context, TlsNoCompression) & TlsNoCompression) ||
                        (g_api.ClearOptions3(created->context, TlsIgnoreUnexpectedEof) & TlsIgnoreUnexpectedEof))
                    {
                        status = EIO;
                    }
                }
                else if (!(g_api.SetOptions11(created->context, TlsNoCompression) & TlsNoCompression))
                {
                    status = EIO;
                }
                if ((0 == status) && (1 != g_api.SSL_CTX_set_default_verify_paths(created->context))) status = EIO;
                if (0 == status) status = TelemetryDeadlineRemaining(deadline, &remaining);
            }
        }
    }
    if (0 != status)
    {
        Failure(NULL, "initialization", status, log);
        TelemetryTlsDestroy(&created, log);
        return status;
    }
    *tls = created;
    OsConfigLogInfo(log, "TelemetryTls: Initialized OS trust and peer verification");
    return 0;
}

int TelemetryTlsHandshake(TelemetryTls* tls, int descriptor, const char* peer,
    int64_t deadline, OsConfigLogHandle log)
{
#ifdef OSCONFIG_TELEMETRY_MINTLS
    if (tls && tls->fallback)
    {
        int signals = WorkerSignals();
        if (signals) return Failure(NULL, "worker signals", signals, log);
        return MinTlsHandshake(tls->fallback, descriptor, peer, deadline, log);
    }
#endif
    int status = 0;
    int remaining = 0;
    unsigned char address[16] = {0};
    bool ip = false;
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
        for (const char* c = peer; *c && !status && !ip; ++c)
        {
            if (!((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') ||
                (*c >= '0' && *c <= '9') || *c == '-' || *c == '.' || *c == '_')) status = EINVAL;
        }
        int flags = fcntl(descriptor, F_GETFL);
        if (flags < 0) status = errno ? errno : EIO;
        else if (!(flags & O_NONBLOCK)) status = EINVAL;
        if (0 == status) status = WorkerSignals();
        if (0 == status) status = TelemetryDeadlineRemaining(deadline, &remaining);
    }
    if (0 != status) return Failure(NULL, "handshake arguments", status, log);

    g_api.ERR_clear_error();
    tls->descriptor = descriptor;
    tls->connection = g_api.SSL_new(tls->context);
    if (NULL == tls->connection) return Failure(tls, "session allocation", EIO, log);
    VerifyParameters* parameters = g_api.SSL_get0_param(tls->connection);
    if ((NULL == parameters) || (1 != g_api.SSL_set_fd(tls->connection, descriptor)))
    {
        return Failure(tls, "socket setup", EIO, log);
    }
    g_api.X509_VERIFY_PARAM_set_hostflags(parameters,
        g_api.X509_VERIFY_PARAM_get_hostflags(parameters) | TlsNoPartialWildcards);
    if (ip)
    {
        if (1 != g_api.X509_VERIFY_PARAM_set1_ip_asc(parameters, peer)) status = EINVAL;
    }
    else if ((1 != g_api.SSL_set1_host(tls->connection, peer)) ||
        (1 != g_api.SSL_ctrl(tls->connection, TlsSetServerName, 0, (void*)peer)))
    {
        status = EIO;
    }
    if (0 != status) return Failure(tls, "peer identity setup", status, log);

    for (;;)
    {
        status = TelemetryDeadlineRemaining(deadline, &remaining);
        if (0 != status) return Failure(tls, "handshake deadline", status, log);
        g_api.ERR_clear_error();
        errno = 0;
        int done = g_api.SSL_connect(tls->connection);
        int systemError = errno;
        if (done == 1) break;
        int sslError = g_api.SSL_get_error(tls->connection, done);
        status = Wait(tls, sslError, systemError, deadline);
        if (0 != status) return Failure(tls, "handshake", status, log);
    }
    Certificate* certificate = g_api.GetPeerCertificate(tls->connection);
    bool verified = (NULL != certificate) && (0 == g_api.SSL_get_verify_result(tls->connection));
    if (NULL != certificate) g_api.X509_free(certificate);
    if (!verified) return Failure(tls, "certificate verification", EACCES, log);
    const unsigned char* protocol = NULL;
    unsigned int protocolSize = 0;
    g_api.SSL_get0_alpn_selected(tls->connection, &protocol, &protocolSize);
    if (protocolSize && (protocolSize != 8 || NULL == protocol || memcmp(protocol, "http/1.1", 8)))
    {
        return Failure(tls, "negotiated HTTP protocol", ENOTSUP, log);
    }
    status = TelemetryDeadlineRemaining(deadline, &remaining);
    if (0 != status) return Failure(tls, "handshake deadline", status, log);
    tls->connected = true;
    OsConfigLogInfo(log, "TelemetryTls: Verified secure connection established");
    return 0;
}

int TelemetryTlsWrite(TelemetryTls* tls, const void* bytes, size_t size,
    int64_t deadline, OsConfigLogHandle log)
{
#ifdef OSCONFIG_TELEMETRY_MINTLS
    if (tls && tls->fallback) return MinTlsWrite(tls->fallback, bytes, size, deadline, log);
#endif
    if ((NULL == tls) || !tls->connected || ((NULL == bytes) && size) || (size > INT_MAX))
    {
        return Failure(NULL, "write arguments", EINVAL, log);
    }
    size_t offset = 0;
    while (offset < size)
    {
        int remaining = 0;
        int status = TelemetryDeadlineRemaining(deadline, &remaining);
        if (0 != status) return Failure(tls, "write deadline", status, log);
        g_api.ERR_clear_error();
        errno = 0;
        int done = g_api.SSL_write(tls->connection, (const unsigned char*)bytes + offset, (int)(size - offset));
        int systemError = errno;
        if (done > 0)
        {
            offset += (size_t)done;
            continue;
        }
        int sslError = g_api.SSL_get_error(tls->connection, done);
        status = Wait(tls, sslError, systemError, deadline);
        if (0 != status) return Failure(tls, "write", status, log);
    }
    int remaining = 0;
    int status = TelemetryDeadlineRemaining(deadline, &remaining);
    return status ? Failure(tls, "write deadline", status, log) : 0;
}

int TelemetryTlsRead(TelemetryTls* tls, void* bytes, size_t capacity, size_t* size,
    bool* endOfStream, int64_t deadline, OsConfigLogHandle log)
{
#ifdef OSCONFIG_TELEMETRY_MINTLS
    if (tls && tls->fallback) return MinTlsRead(tls->fallback, bytes, capacity, size, endOfStream, deadline, log);
#endif
    if (NULL != size) *size = 0;
    if (NULL != endOfStream) *endOfStream = false;
    if ((NULL == tls) || !tls->connected || (NULL == bytes) || !capacity ||
        (capacity > INT_MAX) || (NULL == size) || (NULL == endOfStream))
    {
        return Failure(NULL, "read arguments", EINVAL, log);
    }
    for (;;)
    {
        int remaining = 0;
        int status = TelemetryDeadlineRemaining(deadline, &remaining);
        if (0 != status) return Failure(tls, "read deadline", status, log);
        g_api.ERR_clear_error();
        errno = 0;
        int done = g_api.SSL_read(tls->connection, bytes, (int)capacity);
        int systemError = errno;
        int sslError = (done > 0) ? 0 : g_api.SSL_get_error(tls->connection, done);
        if ((done > 0) || (sslError == TlsClosed))
        {
            status = TelemetryDeadlineRemaining(deadline, &remaining);
            if (0 != status) return Failure(tls, "read deadline", status, log);
            *size = (done > 0) ? (size_t)done : 0;
            *endOfStream = (sslError == TlsClosed);
            if (*endOfStream) Disconnect(tls);
            return 0;
        }
        status = Wait(tls, sslError, systemError, deadline);
        if (0 != status) return Failure(tls, "read", status, log);
    }
}

void TelemetryTlsDestroy(TelemetryTls** tls, OsConfigLogHandle log)
{
    if ((NULL == tls) || (NULL == *tls)) return;
#ifdef OSCONFIG_TELEMETRY_MINTLS
    MinTlsDestroy(&(*tls)->fallback, log);
#endif
    Disconnect(*tls);
    if (NULL != (*tls)->context) g_api.SSL_CTX_free((*tls)->context);
    free(*tls);
    *tls = NULL;
    OsConfigLogInfo(log, "TelemetryTls: Released TLS context");
}
