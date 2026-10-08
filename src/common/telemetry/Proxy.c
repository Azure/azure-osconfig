// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "Proxy.h"
#include "Http.h"

#include <errno.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char* Environment(const char* lower, const char* upper)
{
    const char* value = getenv(lower);

    if ((NULL == value) || ('\0' == *value))
    {
        value = getenv(upper);
    }

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
    size_t offset = 0;
    size_t i = 0;
    unsigned char value = '\0';

    if ((0 != size) && ('.' == token[size - 1]))
    {
        --size;
    }

    if ((0 != size) && ('.' == *token))
    {
        ++token;
        --size;
    }

    if ((0 == size) || (size > hostSize))
    {
        return false;
    }

    offset = hostSize - size;

    if ((0 != offset) && ('.' != host[offset - 1]))
    {
        return false;
    }

    for (i = 0; i < size; ++i)
    {
        value = (unsigned char)token[i];

        if ((value >= 'A') && (value <= 'Z'))
        {
            value += 'a' - 'A';
        }

        if (value != (unsigned char)host[offset + i])
        {
            return false;
        }
    }

    return true;
}

static bool BypassesAria(const char* list)
{
    const char* token = NULL;

    if (0 == strcmp(list, "*"))
    {
        return true;
    }

    while ('\0' != *list)
    {
        while (Blank(*list))
        {
            ++list;
        }

        token = list;

        while (('\0' != *list) && !Blank(*list) && (',' != *list))
        {
            ++list;
        }

        if (DomainMatches(token, (size_t)(list - token)))
        {
            return true;
        }

        while (Blank(*list))
        {
            ++list;
        }

        // curl uses commas, not whitespace, to separate bypass entries.
        if (',' != *list)
        {
            break;
        }

        while (',' == *list)
        {
            ++list;
        }
    }

    return false;
}

int TelemetryProxyDiscover(TelemetryProxySelection* selection, OsConfigLogHandle log)
{
    const char* bypass = NULL;
    const char* proxy = NULL;
    size_t size = 0;

    if (NULL == selection)
    {
        OsConfigLogError(log, "TelemetryProxyDiscover: selection output validation failed with %d (%s)", EINVAL, strerror(EINVAL));
        return EINVAL;
    }

    memset(selection, 0, sizeof(*selection));

    bypass = Environment("no_proxy", "NO_PROXY");

    if (NULL != bypass)
    {
        if (strnlen(bypass, TELEMETRY_PROXY_BYPASS_LIMIT + 1) > TELEMETRY_PROXY_BYPASS_LIMIT)
        {
            OsConfigLogError(log, "TelemetryProxyDiscover: bypass configuration length check failed with %d (%s)", E2BIG, strerror(E2BIG));
            return E2BIG;
        }

        if (BypassesAria(bypass))
        {
            selection->kind = TelemetryProxyDirect;
            return 0;
        }
    }

    proxy = Environment("https_proxy", "HTTPS_PROXY");

    if (NULL == proxy)
    {
        proxy = Environment("all_proxy", "ALL_PROXY");
    }

    if (NULL == proxy)
    {
        selection->kind = TelemetryProxyDirect;
        return 0;
    }

    size = strnlen(proxy, TELEMETRY_PROXY_URL_LIMIT + 1);

    if (size > TELEMETRY_PROXY_URL_LIMIT)
    {
        OsConfigLogError(log, "TelemetryProxyDiscover: proxy configuration length check failed with %d (%s)", E2BIG, strerror(E2BIG));
        return E2BIG;
    }

    memcpy(selection->url, proxy, size + 1);
    selection->kind = TelemetryProxyConfigured;

    return 0;
}

static int Hex(unsigned char c)
{
    int value = -1;

    if ((c >= '0') && (c <= '9'))
    {
        value = c - '0';
    }
    else if ((c >= 'a') && (c <= 'f'))
    {
        value = (c - 'a') + 10;
    }
    else if ((c >= 'A') && (c <= 'F'))
    {
        value = (c - 'A') + 10;
    }

    return value;
}

static int DecodeCredential(const char* begin, const char* end, bool user, char* output, size_t* size)
{
    unsigned char c = '\0';

    *size = 0;

    while (begin < end)
    {
        c = (unsigned char)*begin++;

        if ('%' == c)
        {
            if (((end - begin) < 2) || (Hex(begin[0]) < 0) || (Hex(begin[1]) < 0))
            {
                return EINVAL;
            }

            c = (unsigned char)((Hex(begin[0]) * 16) + Hex(begin[1]));
            begin += 2;
        }

        if ((c < 32) || (127 == c) || (user && (':' == c)))
        {
            return EINVAL;
        }

        if (256 == *size)
        {
            return E2BIG;
        }

        output[(*size)++] = (char)c;
    }

    return 0;
}

static int Credentials(const char* begin, const char* end, char* authorization)
{
    char credentials[513] = {0};
    size_t userSize = 0, passwordSize = 0;
    const char* colon = memchr(begin, ':', (size_t)(end - begin));
    int status = 0;
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t written = 6;
    size_t i = 0;
    unsigned int value = 0;

    if (NULL == colon)
    {
        colon = end;
    }

    if (0 != (status = DecodeCredential(begin, colon, true, credentials, &userSize)))
    {
        return status;
    }

    credentials[userSize] = ':';

    if (0 != (status = DecodeCredential((colon == end) ? end : (colon + 1), end, false, credentials + userSize + 1, &passwordSize)))
    {
        return status;
    }

    const size_t size = userSize + 1 + passwordSize;
    memcpy(authorization, "Basic ", 6);

    for (i = 0; i < size; i += 3)
    {
        value = (unsigned char)credentials[i] << 16;

        if ((i + 1) < size)
        {
            value |= (unsigned char)credentials[i + 1] << 8;
        }

        if ((i + 2) < size)
        {
            value |= (unsigned char)credentials[i + 2];
        }

        authorization[written++] = alphabet[(value >> 18) & 63];
        authorization[written++] = alphabet[(value >> 12) & 63];
        authorization[written++] = ((i + 1) < size) ? alphabet[(value >> 6) & 63] : '=';
        authorization[written++] = ((i + 2) < size) ? alphabet[value & 63] : '=';
    }

    authorization[written] = '\0';

    return 0;
}

static int ParseHttp(const char* url, TelemetryHttpProxy* proxy)
{
    size_t size = 0;
    const char* scheme = NULL;
    const char* end = NULL;
    const char* at = NULL;
    int status = 0;
    const char* hostEnd = NULL;
    const char* port = NULL;
    bool ipv6 = false;
    const char* colon = NULL;
    size_t hostSize = 0;
    unsigned char address[16] = {0};
    char c = '\0';
    unsigned int number = 1080;

    if ((NULL == url) || ('\0' == *url))
    {
        return EINVAL;
    }

    size = strnlen(url, TELEMETRY_PROXY_URL_LIMIT + 1);

    if (size > TELEMETRY_PROXY_URL_LIMIT)
    {
        return E2BIG;
    }

    for (size_t i = 0; i < size; ++i)
    {
        if (((unsigned char)url[i] <= 32) || ((unsigned char)url[i] >= 127))
        {
            return EINVAL;
        }
    }

    scheme = strstr(url, "://");

    if (NULL != scheme)
    {
        if ((4 != (scheme - url)) || (('h' != url[0]) && ('H' != url[0])) || (('t' != url[1]) && ('T' != url[1])) || (('t' != url[2]) && ('T' != url[2])) || (('p' != url[3]) && ('P' != url[3])))
        {
            return ENOTSUP;
        }

        url = scheme + 3;
    }

    end = url + strcspn(url, "/?#");

    if (('\0' != *end) && (('/' != end[0]) || ('\0' != end[1])))
    {
        return ENOTSUP;
    }

    at = memchr(url, '@', (size_t)(end - url));

    if (NULL != at)
    {
        if (0 != (status = Credentials(url, at, proxy->authorization)))
        {
            return status;
        }

        url = at + 1;
    }

    if (url >= end)
    {
        return EINVAL;
    }

    hostEnd = end;
    ipv6 = '[' == *url;

    if (ipv6)
    {
        ++url;
        hostEnd = memchr(url, ']', (size_t)(end - url));

        if (NULL == hostEnd)
        {
            return EINVAL;
        }

        if ((hostEnd + 1) != end)
        {
            if (':' != hostEnd[1])
            {
                return EINVAL;
            }

            port = hostEnd + 2;
        }
    }
    else
    {
        colon = memchr(url, ':', (size_t)(end - url));

        if (NULL != colon)
        {
            hostEnd = colon;
            port = colon + 1;
        }
    }

    hostSize = (size_t)(hostEnd - url);

    if ((0 == hostSize) || (hostSize >= sizeof(proxy->host)))
    {
        return EINVAL;
    }

    memcpy(proxy->host, url, hostSize);
    proxy->host[hostSize] = '\0';

    if (ipv6)
    {
        if (1 != inet_pton(AF_INET6, proxy->host, address))
        {
            return EINVAL;
        }
    }
    else
    {
        for (size_t i = 0; i < hostSize; ++i)
        {
            c = proxy->host[i];

            if (!(((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9')) || ('-' == c) || ('.' == c) || ('_' == c)))
            {
                return EINVAL;
            }
        }
    }

    if (NULL != port)
    {
        if (port == end)
        {
            return EINVAL;
        }

        number = 0;

        while (port < end)
        {
            if ((*port < '0') || (*port > '9'))
            {
                return EINVAL;
            }

            number = (number * 10) + (unsigned int)(*port++ - '0');

            if (number > 65535)
            {
                return EINVAL;
            }
        }

        if (0 == number)
        {
            return EINVAL;
        }
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

        if (0 != (status = ParseHttp(url, proxy)))
        {
            memset(proxy, 0, sizeof(*proxy));
        }
    }

    if (0 != status)
    {
        OsConfigLogError(log, "TelemetryProxyParseHttp: ParseHttp/endpoint validation failed with %d (%s)", status, strerror(status));
    }

    return status;
}

int TelemetryProxyBuildConnect(const TelemetryHttpProxy* proxy, char* bytes, size_t capacity, size_t* size, OsConfigLogHandle log)
{
    int status = EINVAL;
    char request[1024] = {0};
    int count = 0;

    if (NULL != size)
    {
        *size = 0;
    }

    if ((NULL != proxy) && (NULL != bytes) && (NULL != size) && (0 != proxy->port))
    {
        // Only the validated parser output is accepted by this internal builder.
        count = snprintf(request, sizeof(request),
            "CONNECT " TELEMETRY_ARIA_HOST ":443 HTTP/1.1\r\n"
            "Host: " TELEMETRY_ARIA_HOST ":443\r\n%s%s%s\r\n",
            ('\0' != proxy->authorization[0]) ? "Proxy-Authorization: " : "", proxy->authorization, ('\0' != proxy->authorization[0]) ? "\r\n" : "");
        status = (count < 0) ? EIO : ((((size_t)count >= sizeof(request)) || ((size_t)count >= capacity)) ? EMSGSIZE : 0);

        if (0 == status)
        {
            memcpy(bytes, request, (size_t)count + 1);
            *size = (size_t)count;
        }
    }

    if (0 != status)
    {
        OsConfigLogError(log, "TelemetryProxyBuildConnect: CONNECT request formatting failed with %d (%s)", status, strerror(status));
    }

    return status;
}
