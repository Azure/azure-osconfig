// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "MinTls.h"
#include "ctr_drbg.h"
#include "MinTlsLog.h"
#include "entropy.h"
#include "ssl.h"
#include "x509_crt.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

struct MinTls
{
    mbedtls_ssl_context session;
    mbedtls_ssl_config configuration;
    mbedtls_x509_crt roots;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context random;
    int descriptor;
    int ioError;
    unsigned char ip[16];
    size_t ipSize;
    bool connected;
    bool attached;
    OsConfigLogHandle log;
};

int MinTlsRemaining(int64_t deadline, int* milliseconds, OsConfigLogHandle log)
{
    struct timespec now = {0};
    int64_t time = 0;
    int64_t left = 0;
    int status = 0;

    if (clock_gettime(CLOCK_MONOTONIC, &now))
    {
        status = errno ? errno : EIO;
        OsConfigLogError(log, "MinTls: monotonic clock failed, status: %d (%s)", status, strerror(status));
    }
    else if ((now.tv_sec < 0) || ((uint64_t)now.tv_sec > (uint64_t)(INT64_MAX / 1000000000 - 1)))
    {
        status = EOVERFLOW;
        OsConfigLogError(log, "MinTls: monotonic clock cannot be represented in nanoseconds");
    }
    else
    {
        time = (int64_t)now.tv_sec * 1000000000 + now.tv_nsec;

        if (deadline <= time)
        {
            status = ETIMEDOUT;
            OsConfigLogError(log, "MinTls: operation deadline expired");
        }
        else
        {
            left = (deadline - time) / 1000000 + (0 != (deadline - time) % 1000000);
            *milliseconds = left > INT_MAX ? INT_MAX : (int)left;
        }
    }

    return status;
}

void MinTlsDisconnect(MinTls* tls)
{
    mbedtls_ssl_free(&tls->session);
    mbedtls_ssl_init(&tls->session);
    tls->descriptor = -1;
    tls->attached = false;
    tls->connected = false;
}

int MinTlsFailure(MinTls* tls, const char* operation, int status, int core, OsConfigLogHandle log)
{
    OsConfigLogError(log, "MinTls: %s failed, status: %d (%s), core: %d", operation, status, strerror(status), core);

    if (tls)
    {
        MinTlsDisconnect(tls);
    }

    return status;
}

int MinTlsSend(void* context, const unsigned char* bytes, size_t size, MinTlsDiagnostics* diagnostics)
{
    MinTls* tls = context;
    ssize_t count = send(tls->descriptor, bytes, size, MSG_NOSIGNAL);
    int result = MBEDTLS_ERR_SSL_INTERNAL_ERROR;

    if (count > 0)
    {
        result = (int)count;
    }
    else if ((count < 0) && ((EINTR == errno) || (EAGAIN == errno) || (EWOULDBLOCK == errno)))
    {
        result = MBEDTLS_ERR_SSL_WANT_WRITE;
    }
    else
    {
        tls->ioError = count < 0 ? errno : EIO;
        OsConfigLogError(diagnostics->log, "MinTls: socket send failed, status: %d (%s)", tls->ioError, strerror(tls->ioError));
    }

    return result;
}

int MinTlsReceive(void* context, unsigned char* bytes, size_t size, MinTlsDiagnostics* diagnostics)
{
    MinTls* tls = context;
    ssize_t count = recv(tls->descriptor, bytes, size, 0);
    int result = MBEDTLS_ERR_SSL_INTERNAL_ERROR;

    if (count > 0)
    {
        result = (int)count;
    }
    else if ((count < 0) && ((EINTR == errno) || (EAGAIN == errno) || (EWOULDBLOCK == errno)))
    {
        result = MBEDTLS_ERR_SSL_WANT_READ;
    }
    else if (!count)
    {
        result = MBEDTLS_ERR_SSL_CONN_EOF;
        OsConfigLogError(diagnostics->log, "MinTls: socket closed without authenticated TLS close-notify");
    }
    else
    {
        tls->ioError = errno;
        OsConfigLogError(diagnostics->log, "MinTls: socket receive failed, status: %d (%s)", tls->ioError, strerror(tls->ioError));
    }

    return result;
}

int MinTlsWait(MinTls* tls, int core, int64_t deadline)
{
    uint32_t verified = 0;
    int remaining = 0;
    int status = 0;
    struct pollfd item = {0};
    int ready = 0;

    if ((MBEDTLS_ERR_SSL_WANT_READ != core) && (MBEDTLS_ERR_SSL_WANT_WRITE != core))
    {
        verified = mbedtls_ssl_get_verify_result(&tls->session);

        if ((MBEDTLS_ERR_X509_CERT_VERIFY_FAILED == core) ||
            ((0 != verified) && (UINT32_MAX != verified)))
        {
            status = EACCES;
        }
        else
        {
            status = tls->ioError ? tls->ioError : EPROTO;
        }
    }
    else
    {
        while ((0 == status) && (ready <= 0))
        {
            remaining = 0;

            if (0 == (status = MinTlsRemaining(deadline, &remaining, tls->log)))
            {
                item = (struct pollfd){tls->descriptor,
                    (short)(MBEDTLS_ERR_SSL_WANT_READ == core ? POLLIN : POLLOUT), 0};
                ready = poll(&item, 1, remaining);

                if (ready > 0)
                {
                    status = item.revents & POLLNVAL ? EBADF : 0;

                    if (status)
                    {
                        OsConfigLogError(tls->log, "MinTls: poll reported an invalid socket descriptor");
                    }
                }
                else if ((ready < 0) && (EINTR != errno))
                {
                    status = errno ? errno : EIO;
                    OsConfigLogError(tls->log, "MinTls: socket poll failed, status: %d (%s)", status, strerror(status));
                }
            }
        }
    }

    return status;
}

int MinTlsVerifyIp(void* context, mbedtls_x509_crt* certificate, int depth, uint32_t* flags, MinTlsDiagnostics* diagnostics)
{
    const MinTls* tls = context;
    bool matched = false;
    const mbedtls_x509_sequence* san = NULL;

    // Do not allow a numeric DNS SAN/CN to stand in for an iPAddress SAN.
    if ((0 == depth) && (tls->ipSize))
    {
        for (san = &certificate->subject_alt_names; san; san = san->next)
        {
            if ((MBEDTLS_X509_SAN_IP_ADDRESS == (san->buf.tag & MBEDTLS_ASN1_TAG_VALUE_MASK)) &&
                (san->buf.len == tls->ipSize) && (!memcmp(san->buf.p, tls->ip, tls->ipSize)))
            {
                matched = true;
            }
        }

        if (!matched)
        {
            *flags |= MBEDTLS_X509_BADCERT_CN_MISMATCH;
        }
    }

    if (0 != *flags)
    {
        OsConfigLogError(diagnostics->log, "MinTls: certificate verification failed, depth: %d, flags: %u", depth, (unsigned int)*flags);
    }

    return 0;
}

int MinTlsCheckPolicy(OsConfigLogHandle log)
{
    static const char* overrides[] = {
        "OPENSSL_CONF", "OPENSSL_CONF_INCLUDE", "OPENSSL_MODULES",
        "OPENSSL_FIPS", "OPENSSL_FORCE_FIPS_MODE"
    };
    static const char* policies[] = {"/etc/system-fips", "/etc/crypto-policies/config"};
    struct stat info = {0};
    int descriptor = 0;
    char value[3] = {0};
    ssize_t count = 0;
    size_t i = 0;
    int status = 0;

    for (i = 0; (0 == status) && (i < ARRAY_SIZE(overrides)); ++i)
    {
        if (getenv(overrides[i]))
        {
            OsConfigLogError(log, "MinTls: Refusing to bypass explicit OpenSSL configuration (%s)", overrides[i]);
            status = ENOTSUP;
        }
    }

    for (i = 0; (0 == status) && (i < ARRAY_SIZE(policies)); ++i)
    {
        if (!lstat(policies[i], &info))
        {
            OsConfigLogError(log, "MinTls: System cryptographic policy requires the OS provider");
            status = ENOTSUP;
        }
        else if ((ENOENT != errno) && (ENOTDIR != errno))
        {
            status = errno;
            OsConfigLogError(log, "MinTls: Cannot inspect system cryptographic policy, status: %d (%s)", status, strerror(status));
        }
    }

    if (0 == status)
    {
        descriptor = open("/proc/sys/crypto/fips_enabled", O_RDONLY | O_CLOEXEC);

        if (descriptor < 0)
        {
            if (ENOENT != errno)
            {
                status = errno;
                OsConfigLogError(log, "MinTls: Cannot inspect kernel FIPS policy, status: %d (%s)", status, strerror(status));
            }
        }
        else
        {
            do
            {
                count = read(descriptor, value, sizeof(value));
            } while ((count < 0) && (EINTR == errno));

            status = count < 0 ? errno : ((2 == count) && ('0' == value[0]) && ('\n' == value[1])) ? 0 : ENOTSUP;
            close(descriptor);

            if (status)
            {
                OsConfigLogError(log, "MinTls: Kernel FIPS policy prohibits fallback or could not be read, status: %d (%s)", status, strerror(status));
            }
        }
    }

    return status;
}

int MinTlsLoadTrust(MinTls* tls, const char* caFile, int64_t deadline, OsConfigLogHandle log)
{
    MinTlsDiagnostics diagnosticContext = {log, NULL};
    MinTlsDiagnostics* diagnostics = &diagnosticContext;

    static const char* bundles[] = {
        "/etc/pki/tls/certs/ca-bundle.crt",
        "/etc/ssl/certs/ca-certificates.crt",
        "/etc/ssl/ca-bundle.pem",
        "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem",
        "/etc/ssl/cert.pem"
    };
    struct stat info = {0};
    size_t i = 0;
    int descriptor = -1;
    size_t length = 0;
    unsigned char* bytes = NULL;
    size_t size = 0;
    int remaining = 0;
    ssize_t count = 0;
    int core = 0;
    int status = 0;

    if (!caFile)
    {
        caFile = getenv("SSL_CERT_FILE");
    }

    if ((!caFile) && (getenv("SSL_CERT_DIR")))
    {
        OsConfigLogError(log, "MinTls: Directory-only trust override unsupported; configure SSL_CERT_FILE");
        status = ENOTSUP;
    }
    else if (!caFile)
    {
        for (i = 0; (0 == status) && (!caFile) && (i < ARRAY_SIZE(bundles)); ++i)
        {
            if (!stat(bundles[i], &info))
            {
                caFile = bundles[i];
            }
            else if ((ENOENT != errno) && (ENOTDIR != errno))
            {
                status = errno;
                OsConfigLogError(log, "MinTls: Cannot inspect system trust, status: %d (%s)", status, strerror(status));
            }
        }
    }

    if (0 == status)
    {
        if ((!caFile) || (!*caFile))
        {
            status = ENOENT;
            OsConfigLogError(log, "MinTls: No CA bundle configured or found");
        }
        else if ((descriptor = open(caFile, O_RDONLY | O_CLOEXEC | O_NONBLOCK)) < 0)
        {
            status = errno;
            OsConfigLogError(log, "MinTls: Cannot open CA bundle, status: %d (%s)", status, strerror(status));
        }
        else if (fstat(descriptor, &info))
        {
            status = errno;
            OsConfigLogError(log, "MinTls: Cannot inspect open CA bundle, status: %d (%s)", status, strerror(status));
        }
        else if ((!S_ISREG(info.st_mode)) || (info.st_size <= 0) || (info.st_size > 4 * 1024 * 1024))
        {
            status = EINVAL;
            OsConfigLogError(log, "MinTls: CA bundle is empty, oversized, or not a regular file");
        }
        else
        {
            length = (size_t)info.st_size;
            bytes = malloc(length + 1);

            if (!bytes)
            {
                status = ENOMEM;
                OsConfigLogError(log, "MinTls: Cannot allocate CA bundle buffer");
            }
        }
    }

    while (0 == status)
    {
        if (0 != (status = MinTlsRemaining(deadline, &remaining, log)))
        {
            break;
        }

        count = read(descriptor, bytes + size, length + 1 - size);

        if (count < 0)
        {
            if (EINTR == errno)
            {
                continue;
            }

            status = errno;
            break;
        }

        if (!count)
        {
            break;
        }

        size += (size_t)count;

        if (size > length)
        {
            status = EFBIG;
        }
    }

    if (descriptor >= 0)
    {
        close(descriptor);
    }

    if (bytes)
    {
        if ((0 == status) && (size != length))
        {
            status = EIO;
        }

        if (status)
        {
            OsConfigLogError(log, "MinTls: CA bundle read failed or file changed, status: %d (%s)", status, strerror(status));
        }
        else
        {
            bytes[size] = '\0';

            // Positive means partial parsing. Never silently ignore unrecognized trust.
            if ((0 != (core = mbedtls_x509_crt_parse(&tls->roots, bytes, size + 1, diagnostics))) || (!tls->roots.raw.p))
            {
                status = EACCES;
                OsConfigLogError(log, "MinTls: CA bundle parsing failed, core: %d", core);
            }
        }

        free(bytes);
    }

    return status;
}

int MinTlsCreate(MinTls** tls, const char* caFile, int64_t deadline, OsConfigLogHandle log)
{
    MinTlsDiagnostics diagnosticContext = {log, NULL};
    MinTlsDiagnostics* diagnostics = &diagnosticContext;

    int remaining = 0, core = 0;
    int status = EINVAL;
    MinTls* created = NULL;
    static const unsigned char personalization[] = "OSConfig mintls client";
    static const char* protocols[] = {"http/1.1", NULL};

    if ((!tls) || (*tls))
    {
        return MinTlsFailure(NULL, tls ? "duplicate initialization" : "initialization",
            tls ? EALREADY : EINVAL, 0, log);
    }

    if (0 == (status = MinTlsRemaining(deadline, &remaining, log)))
    {
        status = MinTlsCheckPolicy(log);
    }

    if (0 == status)
    {
        created = calloc(1, sizeof(*created));

        if (!created)
        {
            status = ENOMEM;
            OsConfigLogError(log, "MinTls: Cannot allocate TLS context");
        }
        else
        {
            created->descriptor = -1;
            created->log = log;
            mbedtls_ssl_init(&created->session);
            mbedtls_ssl_config_init(&created->configuration);
            mbedtls_x509_crt_init(&created->roots);
            mbedtls_entropy_init(&created->entropy, diagnostics);
            mbedtls_ctr_drbg_init(&created->random);
            status = MinTlsLoadTrust(created, caFile, deadline, log);
        }
    }

    if (0 == status)
    {
        if (0 != (core = mbedtls_ctr_drbg_seed(&created->random, mbedtls_entropy_func, &created->entropy,
            personalization, sizeof(personalization) - 1, diagnostics)))
        {
            status = EIO;
            OsConfigLogError(log, "MinTls: entropy/DRBG initialization failed, core: %d", core);
        }
        else if (0 != (core = mbedtls_ssl_config_defaults(&created->configuration, MBEDTLS_SSL_IS_CLIENT,
            MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)))
        {
            status = EIO;
            OsConfigLogError(log, "MinTls: TLS configuration failed, core: %d", core);
        }
        else
        {
            mbedtls_ssl_conf_authmode(&created->configuration, MBEDTLS_SSL_VERIFY_REQUIRED);
            mbedtls_ssl_conf_rng(&created->configuration, mbedtls_ctr_drbg_random, &created->random);
            mbedtls_ssl_conf_ca_chain(&created->configuration, &created->roots, NULL);
            mbedtls_ssl_conf_min_tls_version(&created->configuration, MBEDTLS_SSL_VERSION_TLS1_2);
            mbedtls_ssl_conf_max_tls_version(&created->configuration, MBEDTLS_SSL_VERSION_TLS1_2);
            mbedtls_ssl_conf_verify(&created->configuration, MinTlsVerifyIp, created);
            core = mbedtls_ssl_conf_alpn_protocols(&created->configuration, protocols, diagnostics);
            status = core ? EIO : MinTlsRemaining(deadline, &remaining, log);
        }
    }

    if (status)
    {
        MinTlsDestroy(&created, log);
        status = MinTlsFailure(NULL, "initialization", status, core, log);
    }
    else
    {
        *tls = created;
        OsConfigLogInfo(log, "MinTls: Initialized in-tree TLS 1.2 client with verified CA trust");
    }

    return status;
}

int MinTlsHandshake(MinTls* tls, int descriptor, const char* peer, int64_t deadline, OsConfigLogHandle log)
{
    MinTlsDiagnostics diagnosticContext = {log, NULL};
    MinTlsDiagnostics* diagnostics = &diagnosticContext;

    int flags = 0;
    int remaining = 0;
    int status = 0;
    const char* c = NULL;
    int core = 0;
    const char* protocol = NULL;

    if ((!tls) || (!peer) || (!*peer) || (strnlen(peer, 254) > 253) || (descriptor < 0))
    {
        return MinTlsFailure(NULL, "handshake arguments", EINVAL, 0, log);
    }

    if (tls->attached)
    {
        return MinTlsFailure(NULL, "duplicate handshake", EALREADY, 0, log);
    }

    tls->log = log;

    flags = fcntl(descriptor, F_GETFL);

    if (flags < 0)
    {
        return MinTlsFailure(NULL, "socket flags", errno, 0, log);
    }

    if (!(flags & O_NONBLOCK))
    {
        return MinTlsFailure(NULL, "blocking socket", EINVAL, 0, log);
    }

    if (0 != (status = MinTlsRemaining(deadline, &remaining, log)))
    {
        return MinTlsFailure(NULL, "handshake deadline", status, 0, log);
    }

    tls->ipSize = 1 == inet_pton(AF_INET, peer, tls->ip) ? 4 :
        1 == inet_pton(AF_INET6, peer, tls->ip) ? 16 : 0;

    if (!tls->ipSize)
    {
        for (c = peer; *c; ++c)
        {
            if (!(((*c >= 'a') && (*c <= 'z')) || ((*c >= 'A') && (*c <= 'Z')) ||
                ((*c >= '0') && (*c <= '9')) || ('-' == *c) || ('.' == *c) || ('_' == *c)))
            {
                return MinTlsFailure(NULL, "peer name", EINVAL, 0, log);
            }
        }
    }

    tls->descriptor = descriptor;
    tls->ioError = 0;

    if (0 != (core = mbedtls_ssl_setup(&tls->session, &tls->configuration, diagnostics)))
    {
        return MinTlsFailure(tls, "session setup", EIO, core, log);
    }

    tls->attached = true;

    if (0 != (core = mbedtls_ssl_set_hostname(&tls->session, tls->ipSize ? NULL : peer, diagnostics)))
    {
        return MinTlsFailure(tls, "peer setup", EIO, core, log);
    }

    mbedtls_ssl_set_bio(&tls->session, tls, MinTlsSend, MinTlsReceive, NULL);

    for (;;)
    {
        if (0 != (status = MinTlsRemaining(deadline, &remaining, log)))
        {
            return MinTlsFailure(tls, "handshake deadline", status, 0, log);
        }

        if (0 == (core = mbedtls_ssl_handshake(&tls->session, diagnostics)))
        {
            break;
        }

        if (0 != (status = MinTlsWait(tls, core, deadline)))
        {
            return MinTlsFailure(tls, "handshake", status, core, log);
        }
    }

    if ((!mbedtls_ssl_get_peer_cert(&tls->session)) || (mbedtls_ssl_get_verify_result(&tls->session)))
    {
        return MinTlsFailure(tls, "peer verification", EACCES, 0, log);
    }

    protocol = mbedtls_ssl_get_alpn_protocol(&tls->session);

    if ((protocol) && (strcmp(protocol, "http/1.1")))
    {
        return MinTlsFailure(tls, "HTTP protocol", ENOTSUP, 0, log);
    }

    if (0 != (status = MinTlsRemaining(deadline, &remaining, log)))
    {
        return MinTlsFailure(tls, "handshake deadline", status, 0, log);
    }

    tls->connected = true;
    OsConfigLogInfo(log, "MinTls: Verified TLS 1.2 connection established");

    return 0;
}

int MinTlsWrite(MinTls* tls, const void* bytes, size_t size, int64_t deadline, OsConfigLogHandle log)
{
    MinTlsDiagnostics diagnosticContext = {log, NULL};
    MinTlsDiagnostics* diagnostics = &diagnosticContext;

    size_t offset = 0;
    size_t chunk = 0;
    int core = 0;
    int remaining = 0;
    int status = 0;

    if ((!tls) || (!tls->connected) || ((!bytes) && (size)) || (size > INT_MAX))
    {
        return MinTlsFailure(NULL, "write arguments", EINVAL, 0, log);
    }

    tls->log = log;

    while (offset < size)
    {
        remaining = 0;

        if (0 != (status = MinTlsRemaining(deadline, &remaining, log)))
        {
            return MinTlsFailure(tls, "write deadline", status, 0, log);
        }

        chunk = size - offset;

        if (chunk > MBEDTLS_SSL_OUT_CONTENT_LEN)
        {
            chunk = MBEDTLS_SSL_OUT_CONTENT_LEN;
        }

        if ((core = mbedtls_ssl_write(&tls->session, (const unsigned char*)bytes + offset, chunk, diagnostics)) > 0)
        {
            offset += (size_t)core;
            continue;
        }

        status = core ? MinTlsWait(tls, core, deadline) : EPROTO;

        if (status)
        {
            return MinTlsFailure(tls, "write", status, core, log);
        }
    }

    remaining = 0;
    status = MinTlsRemaining(deadline, &remaining, log);

    return status ? MinTlsFailure(tls, "write deadline", status, 0, log) : 0;
}

int MinTlsRead(MinTls* tls, void* bytes, size_t capacity, size_t* size,
    bool* endOfStream, int64_t deadline, OsConfigLogHandle log)
{
    MinTlsDiagnostics diagnosticContext = {log, NULL};
    MinTlsDiagnostics* diagnostics = &diagnosticContext;

    int remaining = 0;
    int status = 0;
    int core = 0;

    if (size)
    {
        *size = 0;
    }

    if (endOfStream)
    {
        *endOfStream = false;
    }

    if ((!tls) || (!tls->connected) || (!bytes) || (!capacity) || (capacity > INT_MAX) || (!size) || (!endOfStream))
    {
        return MinTlsFailure(NULL, "read arguments", EINVAL, 0, log);
    }

    tls->log = log;

    for (;;)
    {
        remaining = 0;

        if (0 != (status = MinTlsRemaining(deadline, &remaining, log)))
        {
            return MinTlsFailure(tls, "read deadline", status, 0, log);
        }

        if (((core = mbedtls_ssl_read(&tls->session, bytes, capacity, diagnostics)) > 0) || (MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY == core))
        {
            if (0 != (status = MinTlsRemaining(deadline, &remaining, log)))
            {
                return MinTlsFailure(tls, "read deadline", status, core, log);
            }

            *size = core > 0 ? (size_t)core : 0;
            *endOfStream = MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY == core;

            if (*endOfStream)
            {
                MinTlsDisconnect(tls);
            }

            return 0;
        }

        status = core ? MinTlsWait(tls, core, deadline) : EPROTO;

        if (status)
        {
            return MinTlsFailure(tls, "read", status, core, log);
        }
    }
}

void MinTlsDestroy(MinTls** tls, OsConfigLogHandle log)
{
    if ((!tls) || (!*tls))
    {
        return;
    }

    (*tls)->log = log;
    mbedtls_ssl_free(&(*tls)->session);
    mbedtls_ssl_config_free(&(*tls)->configuration);
    mbedtls_x509_crt_free(&(*tls)->roots);
    mbedtls_ctr_drbg_free(&(*tls)->random);
    mbedtls_entropy_free(&(*tls)->entropy);
    free(*tls);
    *tls = NULL;
    OsConfigLogDebug(log, "MinTls: Released context; borrowed socket left to caller");
}
