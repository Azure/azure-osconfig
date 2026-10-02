// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "TelemetryProxy.h"
#include "TelemetryHttp.h"

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static const char* Environment(const char* lower, const char* upper)
{
    const char* value = getenv(lower);
    if ((NULL == value) || ('\0' == *value)) value = getenv(upper);
    return ((NULL != value) && ('\0' != *value)) ? value : NULL;
}

static bool Blank(char value)
{
    return (' ' == value) || ('\t' == value);
}

static bool DomainMatches(const char* token, size_t size)
{
    static const char host[] = TELEMETRY_ARIA_HOST;
    const size_t hostSize = sizeof(host) - 1;
    if (size && ('.' == token[size - 1])) --size;
    if (size && ('.' == *token))
    {
        ++token;
        --size;
    }
    if ((0 == size) || (size > hostSize)) return false;
    size_t offset = hostSize - size;
    if (offset && ('.' != host[offset - 1])) return false;
    for (size_t i = 0; i < size; ++i)
    {
        unsigned char value = (unsigned char)token[i];
        if ((value >= 'A') && (value <= 'Z')) value += 'a' - 'A';
        if (value != (unsigned char)host[offset + i]) return false;
    }
    return true;
}

static bool BypassesAria(const char* list)
{
    if (0 == strcmp(list, "*")) return true;
    while (*list)
    {
        while (Blank(*list)) ++list;
        const char* token = list;
        while (*list && !Blank(*list) && (',' != *list)) ++list;
        if (DomainMatches(token, (size_t)(list - token))) return true;
        while (Blank(*list)) ++list;
        // curl uses commas, not whitespace, to separate bypass entries.
        if (',' != *list) break;
        while (',' == *list) ++list;
    }
    return false;
}

int TelemetryProxyDiscover(TelemetryProxySelection* selection, OsConfigLogHandle log)
{
    if (NULL == selection)
    {
        OsConfigLogInfo(log, "TelemetryProxy: Missing selection output (status=%d)", EINVAL);
        return EINVAL;
    }
    memset(selection, 0, sizeof(*selection));

    const char* bypass = Environment("no_proxy", "NO_PROXY");
    if (NULL != bypass)
    {
        if (strnlen(bypass, TELEMETRY_PROXY_BYPASS_LIMIT + 1) > TELEMETRY_PROXY_BYPASS_LIMIT)
        {
            OsConfigLogInfo(log, "TelemetryProxy: Bypass configuration exceeds limit (status=%d)", E2BIG);
            return E2BIG;
        }
        if (BypassesAria(bypass))
        {
            selection->kind = TelemetryProxyDirect;
            OsConfigLogInfo(log, "TelemetryProxy: Aria bypass matched; direct route selected");
            return 0;
        }
    }

    const char* proxy = Environment("https_proxy", "HTTPS_PROXY");
    if (NULL == proxy) proxy = Environment("all_proxy", "ALL_PROXY");
    if (NULL == proxy)
    {
        selection->kind = TelemetryProxyDirect;
        OsConfigLogInfo(log, "TelemetryProxy: No HTTPS proxy configured; direct route selected");
        return 0;
    }
    size_t size = strnlen(proxy, TELEMETRY_PROXY_URL_LIMIT + 1);
    if (size > TELEMETRY_PROXY_URL_LIMIT)
    {
        OsConfigLogInfo(log, "TelemetryProxy: Proxy configuration exceeds limit (status=%d)", E2BIG);
        return E2BIG;
    }
    memcpy(selection->url, proxy, size + 1);
    selection->kind = TelemetryProxyConfigured;
    OsConfigLogInfo(log, "TelemetryProxy: Configured proxy selected; endpoint validation required");
    return 0;
}
