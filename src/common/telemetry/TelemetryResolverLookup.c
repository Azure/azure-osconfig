// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "TelemetryResolverProtocol.h"

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>

void TelemetryLookupHost(const char* host, TelemetryResolverReply* reply)
{
    struct addrinfo hints = {0};
    struct addrinfo* addresses = NULL;
    struct addrinfo* next = NULL;

    memset(reply, 0, sizeof(*reply));
    reply->version = TELEMETRY_RESOLVER_PROTOCOL_VERSION;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    reply->lookupError = getaddrinfo(host, NULL, &hints, &addresses);
    if (0 != reply->lookupError)
    {
        reply->error = (EAI_SYSTEM == reply->lookupError) ? (errno ? errno : EIO) : EHOSTUNREACH;
        return;
    }

    for (next = addresses; (NULL != next) && (reply->count < TELEMETRY_RESOLVER_ADDRESS_LIMIT);
        next = next->ai_next)
    {
        TelemetryResolverAddress* address = &reply->addresses[reply->count];
        if ((AF_INET == next->ai_family) && (NULL != next->ai_addr) &&
            (next->ai_addrlen >= sizeof(struct sockaddr_in)))
        {
            const struct sockaddr_in* ipv4 = (const struct sockaddr_in*)next->ai_addr;
            address->family = AF_INET;
            memcpy(address->bytes, &ipv4->sin_addr, sizeof(ipv4->sin_addr));
            ++reply->count;
        }
        else if ((AF_INET6 == next->ai_family) && (NULL != next->ai_addr) &&
            (next->ai_addrlen >= sizeof(struct sockaddr_in6)))
        {
            const struct sockaddr_in6* ipv6 = (const struct sockaddr_in6*)next->ai_addr;
            address->family = AF_INET6;
            address->scopeId = ipv6->sin6_scope_id;
            memcpy(address->bytes, &ipv6->sin6_addr, sizeof(ipv6->sin6_addr));
            ++reply->count;
        }
    }

    freeaddrinfo(addresses);
    if (0 == reply->count)
    {
        reply->error = EHOSTUNREACH;
    }
}
