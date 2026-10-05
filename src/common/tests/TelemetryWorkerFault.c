// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include <TelemetryWorkerProtocol.h>
#include <TelemetryResolverProtocol.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void Hang(void)
{
    for (;;)
    {
        pause();
    }
}

static int Transfer(void* bytes, size_t size, bool writing, bool split)
{
    size_t offset = 0;
    while (offset < size)
    {
        size_t count = split ? 1 : size - offset;
        ssize_t done = writing ?
            send(STDIN_FILENO, (char*)bytes + offset, count, MSG_NOSIGNAL) :
            recv(STDIN_FILENO, (char*)bytes + offset, count, 0);
        if (done > 0)
        {
            offset += (size_t)done;
        }
        else if ((0 == done) || (EINTR != errno))
        {
            return -1;
        }
    }
    return 0;
}

int main(void)
{
    TelemetryWorkerFrame ready = {0};
    ready.magic = TELEMETRY_WORKER_MAGIC;
    ready.version = TELEMETRY_WORKER_VERSION;
    ready.operation = TELEMETRY_WORKER_READY;
#ifdef TELEMETRY_TEST_BAD_READY
    ++ready.version;
#endif
    if (0 != Transfer(&ready, sizeof(ready), true, false))
    {
        return EXIT_FAILURE;
    }
    for (;;)
    {
        TelemetryWorkerFrame request = {0};
        TelemetryWorkerFrame response = {0};
        TelemetryResolverReply reply = {0};
        char host[TELEMETRY_RESOLVER_HOST_LIMIT + 1] = {0};
        if ((0 != Transfer(&request, sizeof(request), false, false)) ||
            (request.size < 1) || (request.size > sizeof(host)) ||
            (0 != Transfer(host, request.size, false, false)) ||
            ('\0' != host[request.size - 1]))
        {
            return EXIT_SUCCESS;
        }
        if (0 == strcmp(host, "hang"))
        {
            Hang();
        }
        if (0 == strcmp(host, "close-hang"))
        {
            close(STDIN_FILENO);
            Hang();
        }
        if (0 == strcmp(host, "exit"))
        {
            return EXIT_FAILURE;
        }
        response = ready;
        response.operation = request.operation;
        response.sequence = request.sequence;
        response.size = sizeof(reply);
        reply.version = TELEMETRY_RESOLVER_PROTOCOL_VERSION;
        reply.count = 1;
        reply.addresses[0].family = AF_INET;
        reply.addresses[0].bytes[0] = 127;
        reply.addresses[0].bytes[3] = 1;
        if (0 == strcmp(host, "wrong-version")) ++response.version;
        if (0 == strcmp(host, "wrong-operation")) ++response.operation;
        if (0 == strcmp(host, "wrong-sequence")) ++response.sequence;
        if (0 == strcmp(host, "huge")) response.size = UINT32_MAX;
        if (0 == strcmp(host, "negative-status")) response.status = -1;
        if (0 == strcmp(host, "bad-count")) reply.count = TELEMETRY_RESOLVER_ADDRESS_LIMIT + 1;
        if (0 == strcmp(host, "bad-family")) reply.addresses[0].family = AF_UNSPEC;
        if (0 == strcmp(host, "short"))
        {
            Transfer(&response, sizeof(response) / 2, true, false);
            return EXIT_FAILURE;
        }
        if (0 == strcmp(host, "short-hang"))
        {
            Transfer(&response, sizeof(response) / 2, true, false);
            Hang();
        }
        unsigned char packet[sizeof(response) + sizeof(reply) + 1] = {0};
        memcpy(packet, &response, sizeof(response));
        memcpy(packet + sizeof(response), &reply, sizeof(reply));
        size_t size = sizeof(response) + sizeof(reply) + (0 == strcmp(host, "extra"));
        if (0 != Transfer(packet, size, true, 0 == strcmp(host, "split")))
        {
            return EXIT_FAILURE;
        }
        if (0 == strcmp(host, "reply-hang"))
        {
            Hang();
        }
    }
}
