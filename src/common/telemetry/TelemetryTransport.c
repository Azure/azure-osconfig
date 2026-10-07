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
    int remaining = 0;
    int status = 0;
    struct pollfd item = {0};
    int ready = 0;

    while ((!status) && (ready <= 0))
    {
        remaining = 0;

        if (0 == (status = TelemetryDeadlineRemaining(deadline, &remaining)))
        {
            item = (struct pollfd){descriptor, events, 0};
            ready = poll(&item, 1, remaining);

            if (ready > 0)
            {
                status = (item.revents & POLLNVAL) ? EBADF : 0;
            }
            else if ((ready < 0) && (EINTR != errno))
            {
                status = errno ? errno : EIO;
            }
        }
    }

    return status;
}

static void Disconnect(TelemetryTransport* transport, OsConfigLogHandle log)
{
    TelemetryTlsDestroy(&transport->tls, log);

    if (transport->descriptor >= 0)
    {
        if (0 != close(transport->descriptor))
        {
            OsConfigLogInfo(log, "TelemetryTransport: Socket close failed (status=%d)", errno);
        }

        transport->descriptor = -1;
    }
}

static int Connect(TelemetryTransport* transport, int64_t deadline, OsConfigLogHandle log)
{
    TelemetryResolverReply reply = {0};
    TelemetryResolvedHost addresses = {0};
    int remaining = 0;
    int status = 0;
    uint16_t port = 0;
    size_t i = 0;
    struct sockaddr_storage* address = NULL;
    struct sockaddr_in ipv4 = {0};
    struct sockaddr_in6 ipv6 = {0};
    int descriptor = 0;
    int error = 0;
    socklen_t size = 0;

    if (0 != (status = TelemetryDeadlineRemaining(deadline, &remaining)))
    {
        return status;
    }

    TelemetryLookupHost(transport->proxied ? transport->proxy.host : TELEMETRY_ARIA_HOST, &reply);

    if (0 != (status = TelemetryDecodeResolverReply(&reply, &addresses)))
    {
        return status;
    }

    port = htons(transport->proxied ? transport->proxy.port : 443);

    for (i = 0; i < addresses.count; ++i)
    {
        if (0 != (status = TelemetryDeadlineRemaining(deadline, &remaining)))
        {
            break;
        }

        address = &addresses.addresses[i];

        if (AF_INET == address->ss_family)
        {
            memcpy(&ipv4, address, sizeof(ipv4));
            ipv4.sin_port = port;
            memcpy(address, &ipv4, sizeof(ipv4));
        }
        else
        {
            memcpy(&ipv6, address, sizeof(ipv6));
            ipv6.sin6_port = port;
            memcpy(address, &ipv6, sizeof(ipv6));
        }

        descriptor = socket(address->ss_family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);

        if (descriptor < 0)
        {
            status = errno ? errno : EIO;
            OsConfigLogInfo(log, "TelemetryTransport: Socket creation failed (index=%zu, status=%d)", i, status);
            continue;
        }

        if (0 == connect(descriptor, (struct sockaddr*)address, addresses.lengths[i]))
        {
            status = 0;
        }
        else if (EINPROGRESS != errno)
        {
            status = errno ? errno : EIO;
        }
        else
        {
            if (0 == (status = WaitSocket(descriptor, POLLOUT, deadline)))
            {
                error = 0;
                size = sizeof(error);
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
        {
            OsConfigLogInfo(log, "TelemetryTransport: Failed candidate close (status=%d)", errno);
        }
    }

    return status;
}

static int PlainTransfer(int descriptor, void* data, size_t size, bool writing, int64_t deadline)
{
    size_t offset = 0;
    int remaining = 0;
    int status = 0;
    ssize_t count = 0;

    while ((!status) && (offset < size))
    {
        remaining = 0;

        if (0 == (status = TelemetryDeadlineRemaining(deadline, &remaining)))
        {
            count = writing ?
                send(descriptor, (char*)data + offset, size - offset, MSG_NOSIGNAL) :
                recv(descriptor, (char*)data + offset, size - offset, 0);

            if (count > 0)
            {
                offset += (size_t)count;
            }
            else if (!count)
            {
                status = EPROTO;
            }
            else if ((EAGAIN == errno) || (EWOULDBLOCK == errno))
            {
                status = WaitSocket(descriptor, writing ? POLLOUT : POLLIN, deadline);
            }
            else if (EINTR != errno)
            {
                status = errno ? errno : EIO;
            }
        }
    }

    return status;
}

static bool FieldCharacter(unsigned char c)
{
    return ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9')) ||
        ((c) && (strchr("!#$%&'*+-.^_`|~", c)));
}

static int Tunnel(TelemetryTransport* transport, int64_t deadline, OsConfigLogHandle log)
{
    char request[1024] = {0};
    size_t requestSize = 0;
    int status = 0;
    size_t total = 0;
    size_t headers = 0;
    unsigned int informational = 0;
    unsigned int code = 0;
    bool first = true;
    char line[TELEMETRY_HTTP_LINE_LIMIT + 1] = {0};
    size_t length = 0;
    unsigned char byte = '\0';
    char* colon = NULL;
    char* c = NULL;

    if (0 != (status = TelemetryProxyBuildConnect(&transport->proxy, request, sizeof(request), &requestSize, log)))
    {
        return status;
    }

    if (0 != (status = PlainTransfer(transport->descriptor, request, requestSize, true, deadline)))
    {
        return status;
    }

    for (;;)
    {
        length = 0;

        for (;;)
        {
            byte = 0;

            // Do not consume tunneled bytes past the CONNECT header boundary.
            if (0 != (status = PlainTransfer(transport->descriptor, &byte, 1, false, deadline)))
            {
                return status;
            }

            if (++total > TELEMETRY_HTTP_HEADER_LIMIT)
            {
                return EMSGSIZE;
            }

            if ('\n' == byte)
            {
                if ((!length) || ('\r' != line[length - 1]))
                {
                    return EPROTO;
                }

                line[--length] = '\0';
                break;
            }

            if (TELEMETRY_HTTP_LINE_LIMIT == length)
            {
                return EMSGSIZE;
            }

            if (((byte < 32) && ('\t' != byte) && ('\r' != byte)) || (byte >= 127) ||
                ((length) && ('\r' == line[length - 1])))
            {
                return EPROTO;
            }

            line[length++] = (char)byte;
        }

        if (first)
        {
            if ((length < 12) || (memcmp(line, "HTTP/1.", 7)) ||
                (('0' != line[7]) && ('1' != line[7])) || (' ' != line[8]) ||
                (line[9] < '1') || (line[9] > '5') || (line[10] < '0') || (line[10] > '9') ||
                (line[11] < '0') || (line[11] > '9') || ((length > 12) && (' ' != line[12])))
            {
                return EPROTO;
            }

            code = (unsigned int)((line[9] - '0') * 100 + (line[10] - '0') * 10 + line[11] - '0');

            if (101 == code)
            {
                return ENOTSUP;
            }

            first = false;
        }
        else if (!length)
        {
            if (code < 200)
            {
                if (++informational > 4)
                {
                    return EMSGSIZE;
                }

                first = true;
            }
            else
            {
                OsConfigLogInfo(log, "TelemetryTransport: CONNECT response (http=%u)", code);
                // Successful CONNECT has no HTTP body, regardless of CL/TE.
                return ((code >= 200) && (code < 300)) ? 0 : 407 == code ? EACCES : ECONNREFUSED;
            }
        }
        else
        {
            if (++headers > 64)
            {
                return EMSGSIZE;
            }

            colon = strchr(line, ':');

            if ((!colon) || (colon == line))
            {
                return EPROTO;
            }

            for (c = line; c < colon; ++c)
            {
                if (!FieldCharacter((unsigned char)*c))
                {
                    return EPROTO;
                }
            }
        }
    }
}

int TelemetryTransportCreate(TelemetryTransport** transport, OsConfigLogHandle log)
{
    int status = EINVAL;
    TelemetryTransport* created = NULL;
    TelemetryProxySelection selection = {0};

    if ((transport) && (*transport))
    {
        status = EALREADY;
    }
    else if (transport)
    {
        created = calloc(1, sizeof(*created));
        status = created ? 0 : ENOMEM;

        if (!status)
        {
            created->descriptor = -1;

            if ((0 == (status = TelemetryProxyDiscover(&selection, log))) && (TelemetryProxyConfigured == selection.kind))
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
    }
    else
    {
        *transport = created;
    }

    return status;
}

static void CaptureSuppression(TelemetryTransport* transport, const TelemetryHttpResponse* response)
{
    size_t i = 0;

    if ((429 == response->status) || (503 == response->status))
    {
        transport->suppressed = true;
    }

    for (i = 0; i < response->controlCount; ++i)
    {
        if (TelemetryTimeDeltaMillis != response->controls[i].kind)
        {
            transport->suppressed = true;
        }
    }
}

int TelemetryTransportSend(TelemetryTransport* transport, const char* token,
    const char* clientVersion, int64_t uploadTimeMilliseconds, const void* event,
    size_t eventSize, int64_t deadline, TelemetryHttpResponse* response, OsConfigLogHandle log)
{
    char headers[TELEMETRY_HTTP_HEADER_LIMIT + 1] = {0};
    size_t headerSize = 0;
    const char* stage = "arguments";
    int status = EINVAL;
    unsigned char bytes[2048] = {0};
    size_t size = 0;
    bool eof = false;
    size_t i = 0;

    if (response)
    {
        TelemetryHttpResponseInitialize(response, log);
    }

    if ((!transport) || (!response) || (!event))
    {
        goto failed;
    }

    if (transport->suppressed)
    {
        status = ECANCELED;
        goto failed;
    }

    if (0 != (status = TelemetryHttpBuildRequest(token, clientVersion, uploadTimeMilliseconds, eventSize,
        headers, sizeof(headers), &headerSize, log)))
    {
        goto failed;
    }

    if (transport->descriptor < 0)
    {
        stage = "DNS/TCP";
        OsConfigLogInfo(log, "TelemetryTransport: Connecting (route=%s)",
            transport->proxied ? "HTTP CONNECT" : "direct");

        if (0 != (status = Connect(transport, deadline, log)))
        {
            goto failed;
        }

        OsConfigLogInfo(log, "TelemetryTransport: TCP connected");

        if (transport->proxied)
        {
            stage = "CONNECT";

            if (0 != (status = Tunnel(transport, deadline, log)))
            {
                goto failed;
            }
        }

        stage = "TLS";

        if (0 == (status = TelemetryTlsCreate(&transport->tls, deadline, log)))
        {
            status = TelemetryTlsHandshake(transport->tls, transport->descriptor,
                TELEMETRY_ARIA_HOST, deadline, log);
        }

        if (status)
        {
            goto failed;
        }
    }

    stage = "request";

    if (0 == (status = TelemetryTlsWrite(transport->tls, headers, headerSize, deadline, log)))
    {
        status = TelemetryTlsWrite(transport->tls, event, eventSize, deadline, log);
    }

    if (status)
    {
        goto failed;
    }

    stage = "response";

    while (!response->complete)
    {
        size = 0;
        eof = false;

        if (0 == (status = TelemetryTlsRead(transport->tls, bytes, sizeof(bytes), &size, &eof, deadline, log)))
        {
            status = TelemetryHttpResponseFeed(response, bytes, size, eof, log);
        }

        if (status)
        {
            goto failed;
        }
    }

    CaptureSuppression(transport, response);

    for (i = 0; i < response->controlCount; ++i)
    {
        if (TelemetryTimeDeltaMillis == response->controls[i].kind)
        {
            OsConfigLogDebug(log, "TelemetryTransport: Clock guidance preserved; sender uses local UTC");
        }
    }

    if ((!response->reusable) || (transport->suppressed))
    {
        Disconnect(transport, log);
    }

    OsConfigLogDebug(log, "TelemetryTransport: Response complete (http=%u, acceptance=%d, controls=%zu)",
        response->status, (int)response->acceptance, response->controlCount);

    return 0;

failed:
    OsConfigLogInfo(log, "TelemetryTransport: %s failed; event not replayed (status=%d)", stage, status);

    if (transport)
    {
        if (response)
        {
            CaptureSuppression(transport, response);
        }

        Disconnect(transport, log);
    }

    return status;
}

bool TelemetryTransportSuppressed(const TelemetryTransport* transport)
{
    return (transport) && (transport->suppressed);
}

void TelemetryTransportDestroy(TelemetryTransport** transport, OsConfigLogHandle log)
{
    if ((transport) && (*transport))
    {
        Disconnect(*transport, log);
        free(*transport);
        *transport = NULL;
    }
}
