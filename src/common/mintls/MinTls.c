// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "MinTls.h"
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>
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
};

static int Remaining(int64_t deadline, int* milliseconds)
{
    struct timespec now = {0};
    if (clock_gettime(CLOCK_MONOTONIC, &now)) return errno ? errno : EIO;
    if (now.tv_sec < 0 || (uint64_t)now.tv_sec > (uint64_t)(INT64_MAX / 1000000000 - 1))
        return EOVERFLOW;
    int64_t time = (int64_t)now.tv_sec * 1000000000 + now.tv_nsec;
    if (deadline <= time) return ETIMEDOUT;
    int64_t left = (deadline - time) / 1000000 + ((deadline - time) % 1000000 != 0);
    *milliseconds = left > INT_MAX ? INT_MAX : (int)left;
    return 0;
}

static void Disconnect(MinTls* tls)
{
    mbedtls_ssl_free(&tls->session);
    mbedtls_ssl_init(&tls->session);
    tls->descriptor = -1;
    tls->attached = false;
    tls->connected = false;
}

static int Failure(MinTls* tls, const char* operation, int status, int core, OsConfigLogHandle log)
{
    OsConfigLogError(log, "MinTls: %s failed (status=%d, core=%d)", operation, status, core);
    if (tls) Disconnect(tls);
    return status;
}

static int Send(void* context, const unsigned char* bytes, size_t size)
{
    MinTls* tls = context;
    ssize_t count = send(tls->descriptor, bytes, size, MSG_NOSIGNAL);
    if (count > 0) return (int)count;
    if (count < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    tls->ioError = count < 0 ? errno : EIO;
    return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
}

static int Receive(void* context, unsigned char* bytes, size_t size)
{
    MinTls* tls = context;
    ssize_t count = recv(tls->descriptor, bytes, size, 0);
    if (count > 0) return (int)count;
    if (count < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
        return MBEDTLS_ERR_SSL_WANT_READ;
    if (!count) return MBEDTLS_ERR_SSL_CONN_EOF;
    tls->ioError = errno;
    return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
}

static int Wait(MinTls* tls, int core, int64_t deadline)
{
    if (core != MBEDTLS_ERR_SSL_WANT_READ && core != MBEDTLS_ERR_SSL_WANT_WRITE)
    {
        uint32_t verified = mbedtls_ssl_get_verify_result(&tls->session);
        if (core == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED ||
            (verified != 0 && verified != UINT32_MAX)) return EACCES;
        return tls->ioError ? tls->ioError : EPROTO;
    }
    for (;;)
    {
        int remaining = 0;
        int status = Remaining(deadline, &remaining);
        if (status) return status;
        struct pollfd item = {tls->descriptor,
            (short)(core == MBEDTLS_ERR_SSL_WANT_READ ? POLLIN : POLLOUT), 0};
        int ready = poll(&item, 1, remaining);
        if (ready > 0) return item.revents & POLLNVAL ? EBADF : 0;
        if (ready < 0 && errno != EINTR) return errno ? errno : EIO;
    }
}

static int VerifyIp(void* context, mbedtls_x509_crt* certificate, int depth, uint32_t* flags)
{
    const MinTls* tls = context;
    if (depth || !tls->ipSize) return 0;
    // Do not allow a numeric DNS SAN/CN to stand in for an iPAddress SAN.
    bool matched = false;
    for (const mbedtls_x509_sequence* san = &certificate->subject_alt_names; san; san = san->next)
    {
        if ((san->buf.tag & MBEDTLS_ASN1_TAG_VALUE_MASK) == MBEDTLS_X509_SAN_IP_ADDRESS &&
            san->buf.len == tls->ipSize && !memcmp(san->buf.p, tls->ip, tls->ipSize))
            matched = true;
    }
    if (!matched) *flags |= MBEDTLS_X509_BADCERT_CN_MISMATCH;
    return 0;
}

static int CheckPolicy(OsConfigLogHandle log)
{
    static const char* overrides[] = {
        "OPENSSL_CONF", "OPENSSL_CONF_INCLUDE", "OPENSSL_MODULES",
        "OPENSSL_FIPS", "OPENSSL_FORCE_FIPS_MODE"
    };
    for (size_t i = 0; i < ARRAY_SIZE(overrides); ++i)
    {
        if (getenv(overrides[i]))
        {
            OsConfigLogError(log, "MinTls: Refusing to bypass explicit OpenSSL configuration (%s)", overrides[i]);
            return ENOTSUP;
        }
    }
    static const char* policies[] = {"/etc/system-fips", "/etc/crypto-policies/config"};
    struct stat info;
    for (size_t i = 0; i < ARRAY_SIZE(policies); ++i)
    {
        if (!lstat(policies[i], &info))
        {
            OsConfigLogError(log, "MinTls: System cryptographic policy requires the OS provider");
            return ENOTSUP;
        }
        if (errno != ENOENT && errno != ENOTDIR)
        {
            int status = errno;
            OsConfigLogError(log, "MinTls: Cannot inspect system cryptographic policy (status=%d)", status);
            return status;
        }
    }
    int descriptor = open("/proc/sys/crypto/fips_enabled", O_RDONLY | O_CLOEXEC);
    if (descriptor < 0)
    {
        if (errno == ENOENT) return 0;
        int status = errno;
        OsConfigLogError(log, "MinTls: Cannot inspect kernel FIPS policy (status=%d)", status);
        return status;
    }
    char value[3] = {0};
    ssize_t count;
    do { count = read(descriptor, value, sizeof(value)); } while (count < 0 && errno == EINTR);
    int status = count < 0 ? errno : count == 2 && value[0] == '0' && value[1] == '\n' ? 0 : ENOTSUP;
    close(descriptor);
    if (status) OsConfigLogError(log, "MinTls: Kernel FIPS policy prohibits fallback or could not be read (status=%d)", status);
    return status;
}

static int LoadTrust(MinTls* tls, const char* caFile, int64_t deadline, OsConfigLogHandle log)
{
    static const char* bundles[] = {
        "/etc/pki/tls/certs/ca-bundle.crt",
        "/etc/ssl/certs/ca-certificates.crt",
        "/etc/ssl/ca-bundle.pem",
        "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem",
        "/etc/ssl/cert.pem"
    };
    if (!caFile) caFile = getenv("SSL_CERT_FILE");
    if (!caFile && getenv("SSL_CERT_DIR"))
    {
        OsConfigLogError(log, "MinTls: Directory-only trust override unsupported; configure SSL_CERT_FILE");
        return ENOTSUP;
    }
    struct stat info;
    if (!caFile)
    {
        for (size_t i = 0; i < ARRAY_SIZE(bundles); ++i)
        {
            if (!stat(bundles[i], &info)) { caFile = bundles[i]; break; }
            if (errno != ENOENT && errno != ENOTDIR)
            {
                int status = errno;
                OsConfigLogError(log, "MinTls: Cannot inspect system trust (status=%d)", status);
                return status;
            }
        }
    }
    if (!caFile || !*caFile)
    {
        OsConfigLogError(log, "MinTls: No CA bundle configured or found");
        return ENOENT;
    }
    int descriptor = open(caFile, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (descriptor < 0)
    {
        int status = errno;
        OsConfigLogError(log, "MinTls: Cannot open CA bundle (status=%d)", status);
        return status;
    }
    if (fstat(descriptor, &info))
    {
        int status = errno;
        close(descriptor);
        OsConfigLogError(log, "MinTls: Cannot inspect open CA bundle (status=%d)", status);
        return status;
    }
    if (!S_ISREG(info.st_mode) || info.st_size <= 0 || info.st_size > 4 * 1024 * 1024)
    {
        close(descriptor);
        OsConfigLogError(log, "MinTls: CA bundle is empty, oversized, or not a regular file");
        return EINVAL;
    }
    size_t length = (size_t)info.st_size;
    unsigned char* bytes = malloc(length + 1);
    if (!bytes)
    {
        close(descriptor);
        OsConfigLogError(log, "MinTls: Cannot allocate CA bundle buffer");
        return ENOMEM;
    }
    size_t size = 0;
    int remaining = 0;
    int status = 0;
    for (;;)
    {
        status = Remaining(deadline, &remaining);
        if (status) break;
        ssize_t count = read(descriptor, bytes + size, length + 1 - size);
        if (count < 0)
        {
            if (errno == EINTR) continue;
            status = errno;
            break;
        }
        if (!count) break;
        size += (size_t)count;
        if (size > length) { status = EFBIG; break; }
    }
    close(descriptor);
    if (!status && size != length) status = EIO;
    if (status)
    {
        free(bytes);
        OsConfigLogError(log, "MinTls: CA bundle read failed or file changed (status=%d)", status);
        return status;
    }
    bytes[size] = '\0';
    int core = mbedtls_x509_crt_parse(&tls->roots, bytes, size + 1);
    free(bytes);
    // Positive means partial parsing. Never silently ignore unrecognized trust.
    if (core || !tls->roots.raw.p)
    {
        OsConfigLogError(log, "MinTls: CA bundle parsing failed (core=%d)", core);
        return EACCES;
    }
    return 0;
}

int MinTlsCreate(MinTls** tls, const char* caFile, int64_t deadline, OsConfigLogHandle log)
{
    int remaining = 0, core = 0;
    int status = EINVAL;
    MinTls* created = NULL;
    if (!tls) goto failed;
    if (*tls) return Failure(NULL, "duplicate initialization", EALREADY, 0, log);
    status = Remaining(deadline, &remaining);
    if (status) goto failed;
    status = CheckPolicy(log);
    if (status) goto failed;
    created = calloc(1, sizeof(*created));
    if (!created) { status = ENOMEM; goto failed; }
    created->descriptor = -1;
    mbedtls_ssl_init(&created->session);
    mbedtls_ssl_config_init(&created->configuration);
    mbedtls_x509_crt_init(&created->roots);
    mbedtls_entropy_init(&created->entropy);
    mbedtls_ctr_drbg_init(&created->random);
    status = LoadTrust(created, caFile, deadline, log);
    if (status) goto failed;
    static const unsigned char personalization[] = "OSConfig mintls client";
    core = mbedtls_ctr_drbg_seed(&created->random, mbedtls_entropy_func, &created->entropy,
        personalization, sizeof(personalization) - 1);
    if (core) { status = EIO; goto failed; }
    core = mbedtls_ssl_config_defaults(&created->configuration, MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (core) { status = EIO; goto failed; }
    mbedtls_ssl_conf_authmode(&created->configuration, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_rng(&created->configuration, mbedtls_ctr_drbg_random, &created->random);
    mbedtls_ssl_conf_ca_chain(&created->configuration, &created->roots, NULL);
    mbedtls_ssl_conf_min_tls_version(&created->configuration, MBEDTLS_SSL_VERSION_TLS1_2);
    mbedtls_ssl_conf_max_tls_version(&created->configuration, MBEDTLS_SSL_VERSION_TLS1_2);
    mbedtls_ssl_conf_verify(&created->configuration, VerifyIp, created);
    static const char* protocols[] = {"http/1.1", NULL};
    core = mbedtls_ssl_conf_alpn_protocols(&created->configuration, protocols);
    if (core) { status = EIO; goto failed; }
    status = Remaining(deadline, &remaining);
    if (status) goto failed;
    *tls = created;
    OsConfigLogInfo(log, "MinTls: Initialized in-tree TLS 1.2 client with verified CA trust");
    return 0;
failed:
    MinTlsDestroy(&created, log);
    return Failure(NULL, "initialization", status, core, log);
}

int MinTlsHandshake(MinTls* tls, int descriptor, const char* peer, int64_t deadline, OsConfigLogHandle log)
{
    if (!tls || !peer || !*peer || strnlen(peer, 254) > 253 || descriptor < 0)
        return Failure(NULL, "handshake arguments", EINVAL, 0, log);
    if (tls->attached) return Failure(NULL, "duplicate handshake", EALREADY, 0, log);
    int flags = fcntl(descriptor, F_GETFL);
    if (flags < 0) return Failure(NULL, "socket flags", errno, 0, log);
    if (!(flags & O_NONBLOCK)) return Failure(NULL, "blocking socket", EINVAL, 0, log);
    int remaining = 0;
    int status = Remaining(deadline, &remaining);
    if (status) return Failure(NULL, "handshake deadline", status, 0, log);
    tls->ipSize = inet_pton(AF_INET, peer, tls->ip) == 1 ? 4 :
        inet_pton(AF_INET6, peer, tls->ip) == 1 ? 16 : 0;
    if (!tls->ipSize)
    {
        for (const char* c = peer; *c; ++c)
            if (!((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') ||
                (*c >= '0' && *c <= '9') || *c == '-' || *c == '.' || *c == '_'))
                return Failure(NULL, "peer name", EINVAL, 0, log);
    }
    tls->descriptor = descriptor;
    tls->ioError = 0;
    int core = mbedtls_ssl_setup(&tls->session, &tls->configuration);
    if (core) return Failure(tls, "session setup", EIO, core, log);
    tls->attached = true;
    core = mbedtls_ssl_set_hostname(&tls->session, tls->ipSize ? NULL : peer);
    if (core) return Failure(tls, "peer setup", EIO, core, log);
    mbedtls_ssl_set_bio(&tls->session, tls, Send, Receive, NULL);
    for (;;)
    {
        status = Remaining(deadline, &remaining);
        if (status) return Failure(tls, "handshake deadline", status, 0, log);
        core = mbedtls_ssl_handshake(&tls->session);
        if (!core) break;
        status = Wait(tls, core, deadline);
        if (status) return Failure(tls, "handshake", status, core, log);
    }
    if (!mbedtls_ssl_get_peer_cert(&tls->session) || mbedtls_ssl_get_verify_result(&tls->session))
        return Failure(tls, "peer verification", EACCES, 0, log);
    const char* protocol = mbedtls_ssl_get_alpn_protocol(&tls->session);
    if (protocol && strcmp(protocol, "http/1.1"))
        return Failure(tls, "HTTP protocol", ENOTSUP, 0, log);
    status = Remaining(deadline, &remaining);
    if (status) return Failure(tls, "handshake deadline", status, 0, log);
    tls->connected = true;
    OsConfigLogInfo(log, "MinTls: Verified TLS 1.2 connection established");
    return 0;
}

int MinTlsWrite(MinTls* tls, const void* bytes, size_t size, int64_t deadline, OsConfigLogHandle log)
{
    if (!tls || !tls->connected || (!bytes && size) || size > INT_MAX)
        return Failure(NULL, "write arguments", EINVAL, 0, log);
    size_t offset = 0;
    while (offset < size)
    {
        int remaining = 0;
        int status = Remaining(deadline, &remaining);
        if (status) return Failure(tls, "write deadline", status, 0, log);
        size_t chunk = size - offset;
        if (chunk > MBEDTLS_SSL_OUT_CONTENT_LEN) chunk = MBEDTLS_SSL_OUT_CONTENT_LEN;
        int core = mbedtls_ssl_write(&tls->session, (const unsigned char*)bytes + offset, chunk);
        if (core > 0) { offset += (size_t)core; continue; }
        status = core ? Wait(tls, core, deadline) : EPROTO;
        if (status) return Failure(tls, "write", status, core, log);
    }
    int remaining = 0;
    int status = Remaining(deadline, &remaining);
    return status ? Failure(tls, "write deadline", status, 0, log) : 0;
}

int MinTlsRead(MinTls* tls, void* bytes, size_t capacity, size_t* size,
    bool* endOfStream, int64_t deadline, OsConfigLogHandle log)
{
    if (size) *size = 0;
    if (endOfStream) *endOfStream = false;
    if (!tls || !tls->connected || !bytes || !capacity || capacity > INT_MAX || !size || !endOfStream)
        return Failure(NULL, "read arguments", EINVAL, 0, log);
    for (;;)
    {
        int remaining = 0;
        int status = Remaining(deadline, &remaining);
        if (status) return Failure(tls, "read deadline", status, 0, log);
        int core = mbedtls_ssl_read(&tls->session, bytes, capacity);
        if (core > 0 || core == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
        {
            status = Remaining(deadline, &remaining);
            if (status) return Failure(tls, "read deadline", status, core, log);
            *size = core > 0 ? (size_t)core : 0;
            *endOfStream = core == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY;
            if (*endOfStream) Disconnect(tls);
            return 0;
        }
        status = core ? Wait(tls, core, deadline) : EPROTO;
        if (status) return Failure(tls, "read", status, core, log);
    }
}

void MinTlsDestroy(MinTls** tls, OsConfigLogHandle log)
{
    if (!tls || !*tls) return;
    mbedtls_ssl_free(&(*tls)->session);
    mbedtls_ssl_config_free(&(*tls)->configuration);
    mbedtls_x509_crt_free(&(*tls)->roots);
    mbedtls_ctr_drbg_free(&(*tls)->random);
    mbedtls_entropy_free(&(*tls)->entropy);
    free(*tls);
    *tls = NULL;
    OsConfigLogDebug(log, "MinTls: Released context; borrowed socket left to caller");
}
