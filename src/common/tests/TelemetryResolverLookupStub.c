// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include <TelemetryResolverProtocol.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

int TelemetryTestGetAddrInfo(const char* host, const char* service,
    const struct addrinfo* hints, struct addrinfo** result)
{
    static struct addrinfo entries[TELEMETRY_RESOLVER_ADDRESS_LIMIT + 3];
    static struct sockaddr_in ipv4[TELEMETRY_RESOLVER_ADDRESS_LIMIT + 3];
    static struct sockaddr_in6 ipv6;
    size_t count = 1;

    (void)service;
    (void)hints;
    *result = NULL;

    if (0 == strcmp(host, "hang"))
    {
        for (;;)
        {
            pause();
        }
    }
    if (0 == strcmp(host, "lookup-failure"))
    {
        return EAI_NONAME;
    }
    if (0 == strcmp(host, "system-failure"))
    {
        errno = EACCES;
        return EAI_SYSTEM;
    }

    memset(entries, 0, sizeof(entries));
    memset(ipv4, 0, sizeof(ipv4));
    memset(&ipv6, 0, sizeof(ipv6));
    if (0 == strcmp(host, "many"))
    {
        count = TELEMETRY_RESOLVER_ADDRESS_LIMIT + 3;
    }
    for (size_t i = 0; i < count; ++i)
    {
        ipv4[i].sin_family = AF_INET;
        ipv4[i].sin_addr.s_addr = htonl(UINT32_C(0x7f000001) + (uint32_t)i);
        entries[i].ai_family = AF_INET;
        entries[i].ai_addr = (struct sockaddr*)&ipv4[i];
        entries[i].ai_addrlen = sizeof(ipv4[i]);
        entries[i].ai_next = (i + 1 < count) ? &entries[i + 1] : NULL;
    }
    if (0 == strcmp(host, "ipv6"))
    {
        ipv6.sin6_family = AF_INET6;
        ipv6.sin6_scope_id = 7;
        ipv6.sin6_addr.s6_addr[15] = 1;
        entries[0].ai_family = AF_INET6;
        entries[0].ai_addr = (struct sockaddr*)&ipv6;
        entries[0].ai_addrlen = sizeof(ipv6);
    }
    if (0 == strcmp(host, "unsupported"))
    {
        entries[0].ai_family = AF_UNSPEC;
    }
    *result = entries;
    return 0;
}

void TelemetryTestFreeAddrInfo(struct addrinfo* result)
{
    (void)result;
}
