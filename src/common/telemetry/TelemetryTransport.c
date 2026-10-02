// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "TelemetryTransport.h"
#include "TelemetryProxy.h"
#include "TelemetryTls.h"
#include "TelemetryDeadline.h"
#include "TelemetryResolver.h"
#include "TelemetryResolverProtocol.h"

#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct TelemetryTransport
{
    TelemetryHttpProxy proxy;
    bool proxied;
    bool suppressed;
    int descriptor;
    TelemetryTls* tls;
};

static int WaitSocket(int descriptor, short events, int64_t deadline)
{
    for (;;)
    {
        int remaining = 0;
        int status = TelemetryDeadlineRemaining(deadline, &remaining);
        if (status) return status;
        struct pollfd item = {descriptor, events, 0};
        int ready = poll(&item, 1, remaining);
        if (ready > 0) return (item.revents & POLLNVAL) ? EBADF : 0;
        if (ready < 0 && errno != EINTR) return errno ? errno : EIO;
    }
}

static void Disconnect(TelemetryTransport* transport, OsConfigLogHandle log)
{
    TelemetryTlsDestroy(&transport->tls, log);
    if (transport->descriptor >= 0)
    {
        if (0 != close(transport->descriptor))
            OsConfigLogInfo(log, "TelemetryTransport: Socket close failed (status=%d)", errno);
        transport->descriptor = -1;
    }
}

static int Connect(TelemetryTransport* transport, int64_t deadline, OsConfigLogHandle log)
{
    TelemetryResolverReply reply;
    TelemetryResolvedHost addresses = {0};
    int remaining = 0;
    int status = TelemetryDeadlineRemaining(deadline, &remaining);
    if (status) return status;
    TelemetryLookupHost(transport->proxied ? transport->proxy.host : TELEMETRY_ARIA_HOST, &reply);
    status = TelemetryDecodeResolverReply(&reply, &addresses);
    if (status) return status;
    uint16_t port = htons(transport->proxied ? transport->proxy.port : 443);
    for (size_t i = 0; i < addresses.count; ++i)
    {
        status = TelemetryDeadlineRemaining(deadline, &remaining);
        if (status) break;
        struct sockaddr_storage* address = &addresses.addresses[i];
        if (address->ss_family == AF_INET)
        {
            struct sockaddr_in ipv4;
            memcpy(&ipv4, address, sizeof(ipv4));
            ipv4.sin_port = port;
            memcpy(address, &ipv4, sizeof(ipv4));
        }
        else
        {
            struct sockaddr_in6 ipv6;
            memcpy(&ipv6, address, sizeof(ipv6));
            ipv6.sin6_port = port;
            memcpy(address, &ipv6, sizeof(ipv6));
        }
        int descriptor = socket(address->ss_family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
        if (descriptor < 0)
        {
            status = errno ? errno : EIO;
            OsConfigLogInfo(log, "TelemetryTransport: Socket creation failed (index=%zu, status=%d)", i, status);
            continue;
        }
        if (0 == connect(descriptor, (struct sockaddr*)address, addresses.lengths[i])) status = 0;
        else if (errno != EINPROGRESS) status = errno ? errno : EIO;
        else
        {
            status = WaitSocket(descriptor, POLLOUT, deadline);
            if (!status)
            {
                int error = 0;
                socklen_t size = sizeof(error);
                status = getsockopt(descriptor, SOL_SOCKET, SO_ERROR, &error, &size) ?
                    (errno ? errno : EIO) : error;
            }
        }
        if (!status)
        {
            transport->descriptor = descriptor;
            return 0;
        }
        OsConfigLogInfo(log, "TelemetryTransport: TCP address failed (index=%zu, status=%d)", i, status);
        if (close(descriptor))
            OsConfigLogInfo(log, "TelemetryTransport: Failed candidate close (status=%d)", errno);
    }
    return status;
}

static int PlainTransfer(int descriptor, void* data, size_t size, bool writing, int64_t deadline)
{
    size_t offset = 0;
    while (offset < size)
    {
        int remaining = 0;
        int status = TelemetryDeadlineRemaining(deadline, &remaining);
        if (status) return status;
        ssize_t count = writing ?
            send(descriptor, (char*)data + offset, size - offset, MSG_NOSIGNAL) :
            recv(descriptor, (char*)data + offset, size - offset, 0);
        if (count > 0) offset += (size_t)count;
        else if (!count) return EPROTO;
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            status = WaitSocket(descriptor, writing ? POLLOUT : POLLIN, deadline);
            if (status) return status;
        }
        else if (errno != EINTR) return errno ? errno : EIO;
    }
    return 0;
}

static bool FieldCharacter(unsigned char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
        (c && strchr("!#$%&'*+-.^_`|~", c));
}

static int Tunnel(TelemetryTransport* transport, int64_t deadline, OsConfigLogHandle log)
{
    char request[1024];
    size_t requestSize = 0;
    int status = TelemetryProxyBuildConnect(&transport->proxy, request, sizeof(request), &requestSize, log);
    if (status) return status;
    status = PlainTransfer(transport->descriptor, request, requestSize, true, deadline);
    if (status) return status;
    size_t total = 0, headers = 0;
    unsigned int informational = 0, code = 0;
    bool first = true;
    for (;;)
    {
        char line[TELEMETRY_HTTP_LINE_LIMIT + 1];
        size_t length = 0;
        for (;;)
        {
            unsigned char byte = 0;
            // Do not consume tunneled bytes past the CONNECT header boundary.
            status = PlainTransfer(transport->descriptor, &byte, 1, false, deadline);
            if (status) return status;
            if (++total > TELEMETRY_HTTP_HEADER_LIMIT) return EMSGSIZE;
            if (byte == '\n')
            {
                if (!length || line[length - 1] != '\r') return EPROTO;
                line[--length] = '\0';
                break;
            }
            if (length == TELEMETRY_HTTP_LINE_LIMIT) return EMSGSIZE;
            if ((byte < 32 && byte != '\t' && byte != '\r') || byte >= 127 ||
                (length && line[length - 1] == '\r')) return EPROTO;
            line[length++] = (char)byte;
        }
        if (first)
        {
            if (length < 12 || memcmp(line, "HTTP/1.", 7) ||
                (line[7] != '0' && line[7] != '1') || line[8] != ' ' ||
                line[9] < '1' || line[9] > '5' || line[10] < '0' || line[10] > '9' ||
                line[11] < '0' || line[11] > '9' || (length > 12 && line[12] != ' ')) return EPROTO;
            code = (unsigned int)((line[9] - '0') * 100 + (line[10] - '0') * 10 + line[11] - '0');
            if (code == 101) return ENOTSUP;
            first = false;
        }
        else if (!length)
        {
            if (code < 200)
            {
                if (++informational > 4) return EMSGSIZE;
                first = true;
            }
            else
            {
                OsConfigLogInfo(log, "TelemetryTransport: CONNECT response (http=%u)", code);
                // Successful CONNECT has no HTTP body, regardless of CL/TE.
                return code >= 200 && code < 300 ? 0 : code == 407 ? EACCES : ECONNREFUSED;
            }
        }
        else
        {
            if (++headers > 64) return EMSGSIZE;
            char* colon = strchr(line, ':');
            if (!colon || colon == line) return EPROTO;
            for (char* c = line; c < colon; ++c) if (!FieldCharacter((unsigned char)*c)) return EPROTO;
        }
    }
}

int TelemetryTransportCreate(TelemetryTransport** transport, OsConfigLogHandle log)
{
    int status = EINVAL;
    TelemetryTransport* created = NULL;
    if (transport && *transport) status = EALREADY;
    else if (transport)
    {
        created = calloc(1, sizeof(*created));
        status = created ? 0 : ENOMEM;
        if (!status)
        {
            created->descriptor = -1;
            TelemetryProxySelection selection;
            status = TelemetryProxyDiscover(&selection, log);
            if (!status && selection.kind == TelemetryProxyConfigured)
            {
                created->proxied = true;
                status = TelemetryProxyParseHttp(selection.url, &created->proxy, log);
            }
        }
    }
    if (status)
    {
        free(created);
        OsConfigLogInfo(log, "TelemetryTransport: Create failed (status=%d)", status);
        return status;
    }
    *transport = created;
    return 0;
}

int TelemetryTransportSend(TelemetryTransport* transport, const char* token,
    const char* clientVersion, int64_t uploadTimeMilliseconds, const void* event,
    size_t eventSize, int64_t deadline, TelemetryHttpResponse* response, OsConfigLogHandle log)
{
    char headers[TELEMETRY_HTTP_HEADER_LIMIT + 1];
    size_t headerSize = 0;
    const char* stage = "arguments";
    int status = EINVAL;
    if (response) TelemetryHttpResponseInitialize(response, log);
    if (!transport || !response || !event) goto failed;
    if (transport->suppressed) { status = ECANCELED; goto failed; }
    status = TelemetryHttpBuildRequest(token, clientVersion, uploadTimeMilliseconds, eventSize,
        headers, sizeof(headers), &headerSize, log);
    if (status) goto failed;
    if (transport->descriptor < 0)
    {
        stage = "DNS/TCP";
        OsConfigLogInfo(log, "TelemetryTransport: Connecting (route=%s)",
            transport->proxied ? "HTTP CONNECT" : "direct");
        status = Connect(transport, deadline, log);
        if (status) goto failed;
        OsConfigLogInfo(log, "TelemetryTransport: TCP connected");
        if (transport->proxied)
        {
            stage = "CONNECT";
            status = Tunnel(transport, deadline, log);
            if (status) goto failed;
        }
        stage = "TLS";
        status = TelemetryTlsCreate(&transport->tls, deadline, log);
        if (!status) status = TelemetryTlsHandshake(transport->tls, transport->descriptor,
            TELEMETRY_ARIA_HOST, deadline, log);
        if (status) goto failed;
    }
    stage = "request";
    status = TelemetryTlsWrite(transport->tls, headers, headerSize, deadline, log);
    if (!status) status = TelemetryTlsWrite(transport->tls, event, eventSize, deadline, log);
    if (status) goto failed;
    stage = "response";
    while (!response->complete)
    {
        unsigned char bytes[2048];
        size_t size = 0;
        bool eof = false;
        status = TelemetryTlsRead(transport->tls, bytes, sizeof(bytes), &size, &eof, deadline, log);
        if (!status) status = TelemetryHttpResponseFeed(response, bytes, size, eof, log);
        if (status) goto failed;
    }
    if (response->status == 429 || response->status == 503) transport->suppressed = true;
    for (size_t i = 0; i < response->controlCount; ++i)
    {
        if (response->controls[i].kind != TelemetryTimeDeltaMillis) transport->suppressed = true;
        else OsConfigLogInfo(log, "TelemetryTransport: Clock guidance preserved; live test uses local UTC");
    }
    if (!response->reusable || transport->suppressed) Disconnect(transport, log);
    OsConfigLogInfo(log, "TelemetryTransport: Response complete (http=%u, acceptance=%d, controls=%zu)",
        response->status, (int)response->acceptance, response->controlCount);
    return 0;

failed:
    OsConfigLogInfo(log, "TelemetryTransport: %s failed; event not replayed (status=%d)", stage, status);
    if (transport) Disconnect(transport, log);
    return status;
}

bool TelemetryTransportSuppressed(const TelemetryTransport* transport)
{
    return transport && transport->suppressed;
}

void TelemetryTransportDestroy(TelemetryTransport** transport, OsConfigLogHandle log)
{
    if (transport && *transport)
    {
        Disconnect(*transport, log);
        free(*transport);
        *transport = NULL;
    }
}
