// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "TelemetryProxy.h"
#include "TelemetryHttp.h"

#include <errno.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
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

        static int Hex(unsigned char c)
        {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        }

        static int DecodeCredential(const char* begin, const char* end, bool user, char* output, size_t* size)
        {
            *size = 0;
            while (begin < end)
            {
                unsigned char c = (unsigned char)*begin++;
                if ('%' == c)
                {
                    if ((end - begin < 2) || (Hex(begin[0]) < 0) || (Hex(begin[1]) < 0)) return EINVAL;
                    c = (unsigned char)(Hex(begin[0]) * 16 + Hex(begin[1]));
                    begin += 2;
                }
                if ((c < 32) || (c == 127) || (user && c == ':')) return EINVAL;
                if (*size == 256) return E2BIG;
                output[(*size)++] = (char)c;
            }
            return 0;
        }

        static int Credentials(const char* begin, const char* end, char* authorization)
        {
            char credentials[513];
            size_t userSize = 0, passwordSize = 0;
            const char* colon = memchr(begin, ':', (size_t)(end - begin));
            if (NULL == colon) colon = end;
            int status = DecodeCredential(begin, colon, true, credentials, &userSize);
            if (status) return status;
            credentials[userSize] = ':';
            status = DecodeCredential(colon == end ? end : colon + 1, end, false,
                credentials + userSize + 1, &passwordSize);
            if (status) return status;
            const size_t size = userSize + 1 + passwordSize;
            static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            memcpy(authorization, "Basic ", 6);
            size_t written = 6;
            for (size_t i = 0; i < size; i += 3)
            {
                unsigned int value = (unsigned char)credentials[i] << 16;
                if (i + 1 < size) value |= (unsigned char)credentials[i + 1] << 8;
                if (i + 2 < size) value |= (unsigned char)credentials[i + 2];
                authorization[written++] = alphabet[(value >> 18) & 63];
                authorization[written++] = alphabet[(value >> 12) & 63];
                authorization[written++] = i + 1 < size ? alphabet[(value >> 6) & 63] : '=';
                authorization[written++] = i + 2 < size ? alphabet[value & 63] : '=';
            }
            authorization[written] = '\0';
            return 0;
        }

        static int ParseHttp(const char* url, TelemetryHttpProxy* proxy)
        {
            if ((NULL == url) || !*url) return EINVAL;
            size_t size = strnlen(url, TELEMETRY_PROXY_URL_LIMIT + 1);
            if (size > TELEMETRY_PROXY_URL_LIMIT) return E2BIG;
            for (size_t i = 0; i < size; ++i)
            {
                if (((unsigned char)url[i] <= 32) || ((unsigned char)url[i] >= 127)) return EINVAL;
            }
            const char* scheme = strstr(url, "://");
            if (NULL != scheme)
            {
                if (scheme - url != 4 || (url[0] != 'h' && url[0] != 'H') ||
                    (url[1] != 't' && url[1] != 'T') || (url[2] != 't' && url[2] != 'T') ||
                    (url[3] != 'p' && url[3] != 'P')) return ENOTSUP;
                url = scheme + 3;
            }
            const char* end = url + strcspn(url, "/?#");
            if (*end && (end[0] != '/' || end[1] != '\0')) return ENOTSUP;
            const char* at = memchr(url, '@', (size_t)(end - url));
            if (NULL != at)
            {
                int status = Credentials(url, at, proxy->authorization);
                if (status) return status;
                url = at + 1;
            }
            const char* hostEnd = end;
            const char* port = NULL;
            bool ipv6 = url < end && *url == '[';
            if (ipv6)
            {
                ++url;
                hostEnd = memchr(url, ']', (size_t)(end - url));
                if (NULL == hostEnd) return EINVAL;
                if (hostEnd + 1 != end)
                {
                    if (hostEnd[1] != ':') return EINVAL;
                    port = hostEnd + 2;
                }
            }
            else
            {
                const char* colon = memchr(url, ':', (size_t)(end - url));
                if (NULL != colon) { hostEnd = colon; port = colon + 1; }
            }
            size_t hostSize = (size_t)(hostEnd - url);
            if (!hostSize || hostSize >= sizeof(proxy->host)) return EINVAL;
            memcpy(proxy->host, url, hostSize);
            proxy->host[hostSize] = '\0';
            if (ipv6)
            {
                unsigned char address[16];
                if (1 != inet_pton(AF_INET6, proxy->host, address)) return EINVAL;
            }
            else
            {
                for (size_t i = 0; i < hostSize; ++i)
                {
                    char c = proxy->host[i];
                    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_')) return EINVAL;
                }
            }
            unsigned int number = 1080;
            if (NULL != port)
            {
                if (port == end) return EINVAL;
                number = 0;
                while (port < end)
                {
                    if (*port < '0' || *port > '9') return EINVAL;
                    number = number * 10 + (unsigned int)(*port++ - '0');
                    if (number > 65535) return EINVAL;
                }
                if (!number) return EINVAL;
            }
            proxy->port = (uint16_t)number;
            return 0;
        }

        int TelemetryProxyParseHttp(const char* url, TelemetryHttpProxy* proxy, OsConfigLogHandle log)
        {
            int status = EINVAL;
            if (NULL != proxy)
            {
                memset(proxy, 0, sizeof(*proxy));
                status = ParseHttp(url, proxy);
                if (status) memset(proxy, 0, sizeof(*proxy));
            }
            if (status) OsConfigLogInfo(log, "TelemetryProxy: Unsupported or invalid endpoint (status=%d)", status);
            return status;
        }

        int TelemetryProxyBuildConnect(const TelemetryHttpProxy* proxy, char* bytes, size_t capacity,
            size_t* size, OsConfigLogHandle log)
        {
            if (NULL != size) *size = 0;
            int status = EINVAL;
            if (proxy && bytes && size && proxy->port)
            {
                // Only the validated parser output is accepted by this internal builder.
                char request[1024];
                int count = snprintf(request, sizeof(request),
                    "CONNECT " TELEMETRY_ARIA_HOST ":443 HTTP/1.1\r\n"
                    "Host: " TELEMETRY_ARIA_HOST ":443\r\n%s%s%s\r\n",
                    proxy->authorization[0] ? "Proxy-Authorization: " : "",
                    proxy->authorization, proxy->authorization[0] ? "\r\n" : "");
                status = (count < 0) ? EIO : ((size_t)count >= sizeof(request) || (size_t)count >= capacity) ?
                    EMSGSIZE : 0;
                if (!status)
                {
                    memcpy(bytes, request, (size_t)count + 1);
                    *size = (size_t)count;
                }
            }
            if (status) OsConfigLogInfo(log, "TelemetryProxy: Cannot format CONNECT (status=%d)", status);
            return status;
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
