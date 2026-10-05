// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include <TelemetryResolverProtocol.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    TelemetryResolverReply reply = {0};
    unsigned char bytes[sizeof(reply) + 1] = {0};
    size_t length = sizeof(reply);
    size_t offset = 0;
    ssize_t size = 0;

    if (argc < 2)
    {
        return EXIT_FAILURE;
    }

    if (0 == strcmp(argv[1], "nonzero-exit"))
    {
        return EXIT_FAILURE;
    }

    if (0 == strcmp(argv[1], "close-then-hang"))
    {
        close(STDOUT_FILENO);

        for (;;)
        {
            pause();
        }
    }

    reply.version = TELEMETRY_RESOLVER_PROTOCOL_VERSION;
    reply.count = 1;
    reply.addresses[0].family = AF_INET;
    reply.addresses[0].bytes[0] = 127;
    reply.addresses[0].bytes[3] = 1;

    if (0 == strcmp(argv[1], "wrong-version"))
    {
        ++reply.version;
    }
    else if (0 == strcmp(argv[1], "bad-count"))
    {
        reply.count = TELEMETRY_RESOLVER_ADDRESS_LIMIT + 1;
    }
    else if (0 == strcmp(argv[1], "bad-family"))
    {
        reply.addresses[0].family = AF_UNSPEC;
    }
    else if (0 == strcmp(argv[1], "truncated"))
    {
        length = 1;
    }
    else if (0 == strcmp(argv[1], "oversized"))
    {
        ++length;
    }

    memcpy(bytes, &reply, sizeof(reply));

    while (offset < length)
    {
        size = write(STDOUT_FILENO, bytes + offset, length - offset);

        if (size > 0)
        {
            offset += (size_t)size;
        }
        else if ((size < 0) && (EINTR == errno))
        {
            continue;
        }
        else
        {
            return EXIT_FAILURE;
        }
    }

    if (0 == strcmp(argv[1], "reply-then-hang"))
    {
        for (;;)
        {
            pause();
        }
    }

    return EXIT_SUCCESS;
}
