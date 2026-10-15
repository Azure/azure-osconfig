// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "AriaInterface.h"
#include "SchemaEncoder.h"
#include "OpenSslAdapter.h"
#include "ExecutionDeadline.h"
#include "Utils.h"

#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <netdb.h>
#include <netinet/in.h>
#include <parson.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum HttpState
{
    HttpStatus,
    HttpHeaders,
    HttpLengthBody,
    HttpEofBody,
    HttpChunkSize,
    HttpChunkBody,
    HttpChunkCr,
    HttpChunkLf,
    HttpTrailers,
    HttpComplete,
    HttpFailed
};

static bool Equal(const char* left, const char* right)
{
    unsigned char a = '\0';
    unsigned char b = '\0';
    bool equal = true;

    for (; ('\0' != *left) && ('\0' != *right); ++left, ++right)
    {
        a = (unsigned char)*left;
        b = (unsigned char)*right;

        if ((a >= 'A') && (a <= 'Z'))
        {
            a += 'a' - 'A';
        }

        if ((b >= 'A') && (b <= 'Z'))
        {
            b += 'a' - 'A';
        }

        if (a != b)
        {
            equal = false;
            break;
        }
    }

    return equal && (*left == *right);
}

static bool TokenCharacter(unsigned char value)
{
    return ((value >= '0') && (value <= '9')) || ((value >= 'a') && (value <= 'z')) || ((value >= 'A') && (value <= 'Z')) || ((0 != value) && (NULL != strchr("!#$%&'*+-.^_`|~", value)));
}

static char* Trim(char* value)
{
    size_t length = 0;

    while ((' ' == *value) || ('\t' == *value))
    {
        ++value;
    }

    length = strlen(value);

    while ((0 != length) && ((' ' == value[length - 1]) || ('\t' == value[length - 1])))
    {
        value[--length] = '\0';
    }

    return value;
}

static bool HeaderValue(const char* value, size_t limit)
{
    size_t size = 0;
    size_t i = 0;
    bool valid = false;

    if ((NULL == value) || ('\0' == *value))
    {
        return false;
    }

    size = strnlen(value, limit + 1);
    valid = size <= limit;

    for (i = 0; valid && (i < size); ++i)
    {
        if (((unsigned char)value[i] <= 32) || ((unsigned char)value[i] >= 127))
        {
            valid = false;
        }
    }

    return valid;
}

int TelemetryHttpBuildRequest(const char* token, const char* clientVersion, int64_t uploadTimeMilliseconds, size_t eventSize, char* headers, size_t capacity, size_t* headerSize, OsConfigLogHandle log)
{
    char formatted[TELEMETRY_HTTP_HEADER_LIMIT + 1] = {0};
    int status = 0;
    int length = 0;
    const char* operation = "argument validation";

    if (NULL != headerSize)
    {
        *headerSize = 0;
    }

    if ((NULL == headerSize) || (NULL == headers) ||
        !HeaderValue(token, TELEMETRY_HTTP_TOKEN_LIMIT) ||
        !HeaderValue(clientVersion, TELEMETRY_HTTP_VERSION_LIMIT) ||
        ((NULL != token) && (NULL != strchr(token, ','))) ||
        (uploadTimeMilliseconds < 0) || (0 == eventSize) ||
        (eventSize > TELEMETRY_MAX_EVENT_SIZE))
    {
        status = EINVAL;
    }
    else
    {
        operation = "snprintf";
        length = snprintf(formatted, sizeof(formatted),
            "POST " TELEMETRY_ARIA_PATH " HTTP/1.1\r\n"
            "Host: " TELEMETRY_ARIA_HOST "\r\n"
            "Content-Type: application/bond-compact-binary\r\n"
            "Content-Length: %zu\r\n"
            "APIKey: %s\r\n"
            "Client-Id: NO_AUTH\r\n"
            "SDK-Version: %s\r\n"
            "Upload-Time: %" PRId64 "\r\n"
            "Accept-Encoding: identity\r\n\r\n",
            eventSize, token, clientVersion, uploadTimeMilliseconds);

        if (length < 0)
        {
            status = EIO;
        }
        else if (((size_t)length >= sizeof(formatted)) || ((size_t)length >= capacity))
        {
            operation = "request buffer size check";
            status = EMSGSIZE;
        }
        else
        {
            memcpy(headers, formatted, (size_t)length + 1);
            *headerSize = (size_t)length;
        }
    }

    if (0 != status)
    {
        OsConfigLogError(log, "TelemetryHttpBuildRequest: %s failed with %d (%s)", operation, status, strerror(status));
    }

    return status;
}

static int Number(const char* text, size_t length, unsigned int base, size_t* value)
{
    size_t number = 0;
    size_t i = 0;
    unsigned int digit = 0;
    unsigned char c = '\0';
    int status = 0;

    if (0 == length)
    {
        return EPROTO;
    }

    for (i = 0; (0 == status) && (i < length); ++i)
    {
        c = (unsigned char)text[i];

        if ((c >= '0') && (c <= '9'))
        {
            digit = c - '0';
        }
        else if ((16 == base) && (c >= 'a') && (c <= 'f'))
        {
            digit = (c - 'a') + 10;
        }
        else if ((16 == base) && (c >= 'A') && (c <= 'F'))
        {
            digit = (c - 'A') + 10;
        }
        else
        {
            status = EPROTO;
        }

        if (0 == status)
        {
            if ((digit >= base) || (number > ((SIZE_MAX - digit) / base)))
            {
                status = EOVERFLOW;
            }
            else
            {
                number = (number * base) + digit;
            }
        }
    }

    if (0 == status)
    {
        *value = number;
    }

    return status;
}

static int Connection(TelemetryHttpResponse* response, char* value)
{
    char* comma = NULL;
    char* token = NULL;
    const char* c = NULL;

    for (;;)
    {
        comma = strchr(value, ',');

        if (NULL != comma)
        {
            *comma = '\0';
        }

        token = Trim(value);

        if ('\0' == *token)
        {
            return EPROTO;
        }

        for (c = token; '\0' != *c; ++c)
        {
            if (!TokenCharacter((unsigned char)*c))
            {
                return EPROTO;
            }
        }

        if (Equal(token, "close"))
        {
            response->connectionClose = true;
        }

        if (Equal(token, "keep-alive"))
        {
            response->connectionKeepAlive = true;
        }

        if (NULL == comma)
        {
            return 0;
        }

        value = comma + 1;
    }
}

static int SaveControl(TelemetryHttpResponse* response, TelemetryHttpControlKind kind, const char* value)
{
    size_t length = strlen(value) + 1;
    TelemetryHttpControl* control = NULL;
    int status = 0;

    if ((response->controlCount >= TELEMETRY_HTTP_CONTROL_LIMIT) || (length > (sizeof(response->controlValues) - response->controlBytes)))
    {
        status = EMSGSIZE;
    }
    else
    {
        control = &response->controls[response->controlCount++];
        control->kind = kind;
        control->offset = response->controlBytes;
        memcpy(response->controlValues + response->controlBytes, value, length);
        response->controlBytes += length;
    }

    return status;
}

static int Header(TelemetryHttpResponse* response, bool trailer)
{
    char* colon = NULL;
    char* nameCharacter = NULL;
    char* value = NULL;
    const char* valueCharacter = NULL;
    const char* name = NULL;
    bool length = false;
    bool transfer = false;
    bool connection = false;
    bool encoding = false;
    bool retry = false;
    bool kills = false;
    bool duration = false;
    bool delta = false;
    size_t parsed = 0;
    int status = 0;

    if (++response->headerCount > 64)
    {
        return EMSGSIZE;
    }

    colon = strchr(response->line, ':');

    if ((NULL == colon) || (colon == response->line))
    {
        return EPROTO;
    }

    for (nameCharacter = response->line; nameCharacter < colon; ++nameCharacter)
    {
        if (!TokenCharacter((unsigned char)*nameCharacter))
        {
            return EPROTO;
        }
    }

    *colon = '\0';
    value = Trim(colon + 1);

    for (valueCharacter = value; '\0' != *valueCharacter; ++valueCharacter)
    {
        if ((((unsigned char)*valueCharacter < 32) && ('\t' != *valueCharacter)) || ((unsigned char)*valueCharacter >= 127))
        {
            return EPROTO;
        }
    }

    name = response->line;
    length = Equal(name, "Content-Length");
    transfer = Equal(name, "Transfer-Encoding");
    connection = Equal(name, "Connection");
    encoding = Equal(name, "Content-Encoding");
    retry = Equal(name, "Retry-After");
    kills = Equal(name, "kill-tokens");
    duration = Equal(name, "kill-duration");
    delta = Equal(name, "time-delta-millis");

    if (trailer)
    {
        // Trailers cannot replace framing, decoding, or collector control decisions.
        return (length || transfer || connection || encoding || retry || kills || duration || delta) ? EPROTO : 0;
    }

    if (length)
    {
        if (0 != (status = Number(value, strlen(value), 10, &parsed)))
        {
            return status;
        }

        if (response->hasLength && (response->contentLength != parsed))
        {
            return EPROTO;
        }

        response->contentLength = parsed;
        response->hasLength = true;
    }
    else if (transfer)
    {
        if (response->chunked)
        {
            return EPROTO;
        }

        if (!Equal(value, "chunked"))
        {
            return ENOTSUP;
        }

        response->chunked = true;
    }
    else if (connection)
    {
        return Connection(response, value);
    }
    else if (encoding)
    {
        if (!Equal(value, "identity"))
        {
            return ENOTSUP;
        }
    }
    else if (response->status >= 200)
    {
        if (retry)
        {
            return SaveControl(response, TelemetryRetryAfter, value);
        }

        if (kills)
        {
            return SaveControl(response, TelemetryKillTokens, value);
        }

        if (duration)
        {
            return SaveControl(response, TelemetryKillDuration, value);
        }

        if (delta)
        {
            return SaveControl(response, TelemetryTimeDeltaMillis, value);
        }
    }

    return 0;
}

static bool JsonWhitespace(char c)
{
    return (' ' == c) || ('\t' == c) || ('\r' == c) || ('\n' == c);
}

static int JsonEnvelope(const char* body, size_t size)
{
    // Parson parses the first value, so bound nesting and require its complete
    // object envelope before calling it. Parson still validates JSON syntax.
    char stack[16] = {0};
    size_t depth = 0;
    bool quoted = false;
    bool escaped = false;
    bool started = false;
    bool ended = false;
    size_t i = 0;
    char c = '\0';

    for (i = 0; i < size; ++i)
    {
        c = body[i];

        if ('\0' == c)
        {
            return EPROTO;
        }

        if (!quoted && ((unsigned char)c < 32) && !JsonWhitespace(c))
        {
            return EPROTO;
        }

        if (ended || !started)
        {
            if (JsonWhitespace(c))
            {
                continue;
            }

            if (ended || ('{' != c))
            {
                return EPROTO;
            }

            started = true;
        }

        if (quoted)
        {
            if (escaped)
            {
                escaped = false;
            }
            else if ('\\' == c)
            {
                escaped = true;
            }
            else if ('"' == c)
            {
                quoted = false;
            }
        }
        else if ('"' == c)
        {
            quoted = true;
        }
        else if (('{' == c) || ('[' == c))
        {
            if (sizeof(stack) == depth)
            {
                return EMSGSIZE;
            }

            stack[depth++] = c;
        }
        else if (('}' == c) || (']' == c))
        {
            if ((0 == depth) || (stack[--depth] != (('}' == c) ? '{' : '[')))
            {
                return EPROTO;
            }

            if (0 == depth)
            {
                ended = true;
            }
        }
    }

    return (ended && !quoted) ? 0 : EPROTO;
}

static int EventFailures(const JSON_Object* object, bool* rejected)
{
    JSON_Value* failures = json_object_get_value(object, "efi");
    JSON_Object* entries = NULL;
    size_t i = 0;
    JSON_Value* entry = NULL;
    const char* value = NULL;
    JSON_Array* indices = NULL;
    size_t j = 0;
    JSON_Value* index = NULL;

    if (NULL == failures)
    {
        return 0;
    }

    entries = json_value_get_object(failures);

    if (NULL == entries)
    {
        return EPROTO;
    }

    for (i = 0; i < json_object_get_count(entries); ++i)
    {
        entry = json_object_get_value_at(entries, i);

        if (JSONString == json_value_get_type(entry))
        {
            value = json_value_get_string(entry);

            if ((3 != json_value_get_string_len(entry)) || (0 != strcmp(value, "all")))
            {
                return EPROTO;
            }

            *rejected = true;
        }
        else if (JSONArray == json_value_get_type(entry))
        {
            indices = json_value_get_array(entry);

            for (j = 0; j < json_array_get_count(indices); ++j)
            {
                index = json_array_get_value(indices, j);

                if ((JSONNumber != json_value_get_type(index)) || (0 != json_value_get_number(index)))
                {
                    return EPROTO;
                }

                *rejected = true;
            }
        }
        else
        {
            return EPROTO;
        }
    }

    return 0;
}

static const char* JsonType(const JSON_Value* value)
{
    const char* type = "missing";

    if (NULL != value)
    {
        switch (json_value_get_type(value))
        {
            case JSONNull:
                type = "null";
                break;
            case JSONString:
                type = "string";
                break;
            case JSONNumber:
                type = "number";
                break;
            case JSONObject:
                type = "object";
                break;
            case JSONArray:
                type = "array";
                break;
            case JSONBoolean:
                type = "boolean";
                break;
            default:
                type = "invalid";
                break;
        }
    }

    return type;
}

static int Acknowledge(TelemetryHttpResponse* response, OsConfigLogHandle log)
{
    int status = 0;
    JSON_Value* utf8 = NULL;
    JSON_Value* value = NULL;
    JSON_Object* object = NULL;
    JSON_Value* accepted = NULL;
    JSON_Value* rejected = NULL;
    double acc = 0.0;
    double rej = 0.0;
    bool failed = false;

    if (200 != response->status)
    {
        response->acceptance = TelemetryRejected;
        OsConfigLogError(log, "Acknowledge: HTTP status check failed with %d (%s); HTTP status %u", ECANCELED, strerror(ECANCELED), response->status);
        return 0;
    }

    if (0 == response->bodySize)
    {
        response->acceptance = TelemetryUnconfirmed;
        OsConfigLogError(log, "Acknowledge: collector acknowledgment check failed with %d (%s); empty response body", EPROTO, strerror(EPROTO));
        return 0;
    }

    if (0 != (status = JsonEnvelope(response->body, response->bodySize)))
    {
        OsConfigLogError(log, "Acknowledge: JsonEnvelope failed with %d (%s); bodyBytes=%zu", status, strerror(status), response->bodySize);
        return status;
    }

    // Parson's string constructor validates UTF-8; its JSON parser does not.
    utf8 = json_value_init_string_with_len(response->body, response->bodySize);

    if (NULL == utf8)
    {
        OsConfigLogError(log, "Acknowledge: json_value_init_string_with_len failed with %d (%s); UTF-8 validation or allocation failure", EPROTO, strerror(EPROTO));
        return EPROTO;
    }

    json_value_free(utf8);
    value = json_parse_string(response->body);

    if (NULL == value)
    {
        OsConfigLogError(log, "Acknowledge: json_parse_string failed with %d (%s); JSON parse or allocation failure", EPROTO, strerror(EPROTO));
        return EPROTO;
    }

    object = json_value_get_object(value);
    accepted = json_object_get_value(object, "acc");
    rejected = json_object_get_value(object, "rej");
    // The collector omits zero counts; a present count must still be numeric.
    acc = (NULL != accepted) ? json_value_get_number(accepted) : 0;
    rej = (NULL != rejected) ? json_value_get_number(rejected) : 0;
    failed = NULL != json_object_get_value(object, "TokenCrackingFailure");

    if (((NULL != accepted) && (JSONNumber != json_value_get_type(accepted))) || ((NULL != rejected) && (JSONNumber != json_value_get_type(rejected))) ||
        !(((0 == acc) && (1 == rej)) || ((1 == acc) && (0 == rej))))
    {
        status = EPROTO;
        OsConfigLogError(log, "Acknowledge: acceptance counts validation failed with %d (%s); accType=%s, acc=%.17g, rejType=%s, rej=%.17g, efiType=%s, tokenFailure=%d",
            status, strerror(status), JsonType(accepted), acc, JsonType(rejected), rej, JsonType(json_object_get_value(object, "efi")), (int)failed);
    }
    else if (0 == (status = EventFailures(object, &failed)))
    {
        response->acceptance = (failed || (1 == rej)) ? TelemetryRejected : TelemetryAccepted;

        if (TelemetryRejected == response->acceptance)
        {
            OsConfigLogError(log, "Acknowledge: collector acceptance check failed with %d (%s)", ECANCELED, strerror(ECANCELED));
        }
    }
    else
    {
        OsConfigLogError(log, "Acknowledge: EventFailures failed with %d (%s); efiType=%s", status, strerror(status), JsonType(json_object_get_value(object, "efi")));
    }

    json_value_free(value);

    return status;
}

static int Complete(TelemetryHttpResponse* response, OsConfigLogHandle log)
{
    int status = 0;

    response->body[response->bodySize] = '\0';

    if (0 == (status = Acknowledge(response, log)))
    {
        response->complete = true;
        response->state = HttpComplete;
        response->reusable = !response->connectionClose && ((1 == response->minorVersion) || response->connectionKeepAlive);
    }

    return status;
}

static int EndHeaders(TelemetryHttpResponse* response, OsConfigLogHandle log)
{
    if (response->hasLength && response->chunked)
    {
        return EPROTO;
    }

    if (response->status < 200)
    {
        if (101 == response->status)
        {
            return ENOTSUP;
        }

        if (response->hasLength || response->chunked)
        {
            return EPROTO;
        }

        if (++response->informationalCount > 4)
        {
            return EMSGSIZE;
        }

        response->state = HttpStatus;
        response->connectionClose = false;
        response->connectionKeepAlive = false;

        return 0;
    }

    if (204 == response->status)
    {
        if (response->hasLength || response->chunked)
        {
            return EPROTO;
        }

        return Complete(response, log);
    }

    if (304 == response->status)
    {
        return Complete(response, log);
    }

    if (response->chunked)
    {
        if (1 != response->minorVersion)
        {
            return EPROTO;
        }

        response->state = HttpChunkSize;
    }
    else if (response->hasLength)
    {
        if (response->contentLength > TELEMETRY_HTTP_BODY_LIMIT)
        {
            return EMSGSIZE;
        }

        response->remaining = response->contentLength;
        response->state = HttpLengthBody;

        if (0 == response->remaining)
        {
            return Complete(response, log);
        }
    }
    else
    {
        response->connectionClose = true;
        response->state = HttpEofBody;
    }

    return 0;
}

static int ChunkSize(TelemetryHttpResponse* response)
{
    char* end = response->line;
    int status = 0;

    while (((*end >= '0') && (*end <= '9')) || ((*end >= 'a') && (*end <= 'f')) || ((*end >= 'A') && (*end <= 'F')))
    {
        ++end;
    }

    if (0 != (status = Number(response->line, (size_t)(end - response->line), 16, &response->remaining)))
    {
        return status;
    }

    // Accept bounded RFC chunk extensions, including quoted values/escapes.
    while ('\0' != *end)
    {
        while ((' ' == *end) || ('\t' == *end))
        {
            ++end;
        }

        if (';' != *end++)
        {
            return EPROTO;
        }

        while ((' ' == *end) || ('\t' == *end))
        {
            ++end;
        }

        if (!TokenCharacter((unsigned char)*end))
        {
            return EPROTO;
        }

        while (TokenCharacter((unsigned char)*end))
        {
            ++end;
        }

        while ((' ' == *end) || ('\t' == *end))
        {
            ++end;
        }

        if ('=' == *end)
        {
            ++end;

            while ((' ' == *end) || ('\t' == *end))
            {
                ++end;
            }

            if ('"' == *end)
            {
                ++end;

                while (('\0' != *end) && ('"' != *end))
                {
                    if ('\\' == *end)
                    {
                        ++end;
                    }

                    if ((((unsigned char)*end < 32) && ('\t' != *end)) || ((unsigned char)*end >= 127))
                    {
                        return EPROTO;
                    }

                    ++end;
                }

                if ('"' != *end++)
                {
                    return EPROTO;
                }
            }
            else
            {
                if (!TokenCharacter((unsigned char)*end))
                {
                    return EPROTO;
                }

                while (TokenCharacter((unsigned char)*end))
                {
                    ++end;
                }
            }
        }
    }

    if (response->remaining > (TELEMETRY_HTTP_BODY_LIMIT - response->bodySize))
    {
        return EMSGSIZE;
    }

    response->state = (0 != response->remaining) ? HttpChunkBody : HttpTrailers;

    return 0;
}

static int Line(TelemetryHttpResponse* response, OsConfigLogHandle log)
{
    size_t i = 0;
    unsigned char c = '\0';

    switch (response->state)
    {
        case HttpStatus:
            if ((response->lineSize < 13) || (0 != memcmp(response->line, "HTTP/1.", 7)) ||
                (('0' != response->line[7]) && ('1' != response->line[7])) ||
                (' ' != response->line[8]) || (response->line[9] < '1') ||
                (response->line[9] > '5') || (response->line[10] < '0') ||
                (response->line[10] > '9') || (response->line[11] < '0') ||
                (response->line[11] > '9') || (' ' != response->line[12]))
            {
                return EPROTO;
            }

            for (i = 13; i < response->lineSize; ++i)
            {
                c = (unsigned char)response->line[i];

                if (((c < 32) && ('\t' != c)) || (c >= 127))
                {
                    return EPROTO;
                }
            }

            response->minorVersion = response->line[7] - '0';
            response->status = ((unsigned int)(response->line[9] - '0') * 100) + ((unsigned int)(response->line[10] - '0') * 10) + (unsigned int)(response->line[11] - '0');
            response->state = HttpHeaders;

            return 0;
        case HttpHeaders:
            return (0 != response->lineSize) ? Header(response, false) : EndHeaders(response, log);
        case HttpChunkSize:
            return ChunkSize(response);
        case HttpTrailers:
            return (0 != response->lineSize) ? Header(response, true) : Complete(response, log);
        default:
            return EPROTO;
    }
}

static int Byte(TelemetryHttpResponse* response, unsigned char byte, OsConfigLogHandle log)
{
    int status = 0;

    if (++response->wireBytes > TELEMETRY_HTTP_WIRE_LIMIT)
    {
        return EMSGSIZE;
    }

    switch (response->state)
    {
        case HttpStatus:
        case HttpHeaders:
        case HttpChunkSize:
        case HttpTrailers:
            if ((HttpChunkSize != response->state) && (++response->headerBytes > TELEMETRY_HTTP_HEADER_LIMIT))
            {
                return EMSGSIZE;
            }

            if (response->carriageReturn)
            {
                if ('\n' != byte)
                {
                    return EPROTO;
                }

                response->carriageReturn = false;
                response->line[response->lineSize] = '\0';
                status = Line(response, log);
                response->lineSize = 0;

                return status;
            }

            if ((0 == byte) || ('\n' == byte))
            {
                return EPROTO;
            }

            if ('\r' == byte)
            {
                response->carriageReturn = true;
                return 0;
            }

            if (TELEMETRY_HTTP_LINE_LIMIT == response->lineSize)
            {
                return EMSGSIZE;
            }

            response->line[response->lineSize++] = (char)byte;

            return 0;
        case HttpLengthBody:
        case HttpEofBody:
        case HttpChunkBody:
            if (TELEMETRY_HTTP_BODY_LIMIT == response->bodySize)
            {
                return EMSGSIZE;
            }

            response->body[response->bodySize++] = (char)byte;

            if (HttpEofBody != response->state)
            {
                if (0 == --response->remaining)
                {
                    if (HttpLengthBody == response->state)
                    {
                        return Complete(response, log);
                    }

                    response->state = HttpChunkCr;
                }
            }

            return 0;
        case HttpChunkCr:
            if ('\r' != byte)
            {
                return EPROTO;
            }

            response->state = HttpChunkLf;

            return 0;
        case HttpChunkLf:
            if ('\n' != byte)
            {
                return EPROTO;
            }

            response->state = HttpChunkSize;

            return 0;
        default:
            return EPROTO;
    }
}

int TelemetryHttpResponseInitialize(TelemetryHttpResponse* response, OsConfigLogHandle log)
{
    if (NULL == response)
    {
        OsConfigLogError(log, "TelemetryHttpResponseInitialize: response pointer validation failed with %d (%s)", EINVAL, strerror(EINVAL));
        return EINVAL;
    }

    memset(response, 0, sizeof(*response));
    response->state = HttpStatus;

    return 0;
}

int TelemetryHttpResponseFeed(TelemetryHttpResponse* response, const void* bytes, size_t size, bool endOfStream, OsConfigLogHandle log)
{
    int status = 0;
    const unsigned char* input = NULL;
    const char* operation = "argument validation";

    if ((NULL == response) || ((NULL == bytes) && (0 != size)) || ((NULL != response) && (HttpFailed == response->state)))
    {
        status = EINVAL;
    }
    else
    {
        input = bytes;
        operation = "Byte";

        for (size_t i = 0; (i < size) && (0 == status); ++i)
        {
            status = Byte(response, input[i], log);
        }

        if ((0 == status) && endOfStream)
        {
            if (HttpEofBody == response->state)
            {
                operation = "Complete";
                status = Complete(response, log);
            }
            else if (!response->complete)
            {
                operation = "end-of-stream framing check";
                status = EPROTO;
            }

            response->reusable = false;
        }
    }

    if (0 != status)
    {
        if (NULL != response)
        {
            OsConfigLogError(log, "TelemetryHttpResponseFeed: %s failed with %d (%s); state=%u, http=%u, bodyBytes=%zu, wireBytes=%zu, hasLength=%d, contentLength=%zu, chunked=%d, remaining=%zu, eof=%d, controls=%zu",
                operation, status, strerror(status), response->state, response->status, response->bodySize, response->wireBytes, (int)response->hasLength, response->contentLength,
                (int)response->chunked, response->remaining, (int)endOfStream, response->controlCount);

            response->state = HttpFailed;
            response->complete = false;
            response->reusable = false;
            response->acceptance = TelemetryUnconfirmed;
        }
        else
        {
            OsConfigLogError(log, "TelemetryHttpResponseFeed: %s failed with %d (%s)", operation, status, strerror(status));
        }
    }

    return status;
}

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

#ifdef TELEMETRY_TEST_POLL
// Redirect only after libc headers, so fortified aliases cannot bypass the mock.
int TelemetryTransportTestPoll(struct pollfd* items, nfds_t count, int milliseconds);
#define poll TelemetryTransportTestPoll
#endif

struct TelemetryTransport
{
    TelemetryHttpProxy proxy;
    bool proxied;
    bool suppressed;
    int descriptor;
    TelemetryTls* tls;
};

static int WaitSocket(int descriptor, short events, int64_t deadline, const char** operation)
{
    int remaining = 0;
    int status = 0;
    struct pollfd item = {0};
    int ready = 0;

    while ((0 == status) && (ready <= 0))
    {
        remaining = 0;
        *operation = "TelemetryDeadlineRemaining";

        if (0 == (status = TelemetryDeadlineRemaining(deadline, &remaining)))
        {
            item = (struct pollfd){descriptor, events, 0};
            *operation = "poll";
            ready = poll(&item, 1, remaining);

            if (ready > 0)
            {
                status = (0 != (item.revents & POLLNVAL)) ? EBADF : 0;
            }
            else if ((ready < 0) && (EINTR != errno))
            {
                status = (0 != errno) ? errno : EIO;
            }
        }
    }

    return status;
}

static void Disconnect(TelemetryTransport* transport, OsConfigLogHandle log)
{
    int status = 0;
    TelemetryTlsDestroy(&transport->tls, log);

    if (transport->descriptor >= 0)
    {
        if (0 != close(transport->descriptor))
        {
            status = errno;
            OsConfigLogError(log, "Disconnect: close failed with %d (%s)", status, strerror(status));
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
    int64_t now = 0;
    int64_t addressDeadline = 0;
    size_t i = 0;
    struct sockaddr_storage* address = NULL;
    struct sockaddr_in ipv4 = {0};
    struct sockaddr_in6 ipv6 = {0};
    int descriptor = 0;
    int error = 0;
    socklen_t size = 0;
    const char* operation = "connect";

    if (0 != (status = TelemetryDeadlineRemaining(deadline, &remaining)))
    {
        OsConfigLogError(log, "Connect: TelemetryDeadlineRemaining failed with %d (%s)", status, strerror(status));
        return status;
    }

    TelemetryLookupHost(transport->proxied ? transport->proxy.host : TELEMETRY_ARIA_HOST, &reply);

    if (0 != (status = TelemetryDecodeResolverReply(&reply, &addresses)))
    {
        OsConfigLogError(log, "Connect: TelemetryDecodeResolverReply failed with %d (%s); getaddrinfo returned %d (%s)",
            status, strerror(status), reply.lookupError, (0 != reply.lookupError) ? gai_strerror(reply.lookupError) : "no resolver error");
        return status;
    }

    port = htons(transport->proxied ? transport->proxy.port : 443);

    for (i = 0; i < addresses.count; ++i)
    {
        operation = "TelemetryMonotonicTime";
        if (0 != (status = TelemetryMonotonicTime(&now)))
        {
            break;
        }

        if (now >= deadline)
        {
            operation = "connection deadline check";
            status = ETIMEDOUT;
            break;
        }

        // Leave a share of the unchanged operation deadline for every remaining address.
        addressDeadline = now + ((deadline - now) / (int64_t)(addresses.count - i));
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

        operation = "socket";
        descriptor = socket(address->ss_family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);

        if (descriptor < 0)
        {
            status = (0 != errno) ? errno : EIO;
            continue;
        }

        operation = "connect";
        if (0 == connect(descriptor, (struct sockaddr*)address, addresses.lengths[i]))
        {
            status = 0;
        }
        else if (EINPROGRESS != errno)
        {
            status = (0 != errno) ? errno : EIO;
        }
        else
        {
            if (0 == (status = WaitSocket(descriptor, POLLOUT, addressDeadline, &operation)))
            {
                operation = "getsockopt(SO_ERROR)";
                error = 0;
                size = sizeof(error);

                if (0 != getsockopt(descriptor, SOL_SOCKET, SO_ERROR, &error, &size))
                {
                    status = (0 != errno) ? errno : EIO;
                }
                else
                {
                    operation = "connect";
                    status = error;
                }
            }
        }

        if (0 == status)
        {
            transport->descriptor = descriptor;
            break;
        }

        OsConfigLogInfo(log, "Connect: address %zu of %zu failed at %s with %d (%s)", i + 1, addresses.count, operation, status, strerror(status));
        if (0 != close(descriptor))
        {
            error = errno;
            OsConfigLogError(log, "Connect: close failed with %d (%s)", error, strerror(error));
        }
    }

    if (0 != status)
    {
        OsConfigLogError(log, "Connect: %s failed with %d (%s); resolved addresses=%zu", operation, status, strerror(status), addresses.count);
    }

    return status;
}

static int PlainTransfer(int descriptor, void* data, size_t size, bool writing, int64_t deadline, OsConfigLogHandle log)
{
    size_t offset = 0;
    int remaining = 0;
    int status = 0;
    ssize_t count = 0;
    const char* operation = "TelemetryDeadlineRemaining";

    while ((0 == status) && (offset < size))
    {
        remaining = 0;
        operation = "TelemetryDeadlineRemaining";

        if (0 == (status = TelemetryDeadlineRemaining(deadline, &remaining)))
        {
            operation = writing ? "send" : "recv";
            count = writing ? send(descriptor, (char*)data + offset, size - offset, MSG_NOSIGNAL) : recv(descriptor, (char*)data + offset, size - offset, 0);

            if (count > 0)
            {
                offset += (size_t)count;
            }
            else if (0 == count)
            {
                status = EPROTO;
            }
            else if ((EAGAIN == errno) || (EWOULDBLOCK == errno))
            {
                status = WaitSocket(descriptor, writing ? POLLOUT : POLLIN, deadline, &operation);
            }
            else if (EINTR != errno)
            {
                status = (0 != errno) ? errno : EIO;
            }
        }
    }

    if (0 != status)
    {
        OsConfigLogError(log, "PlainTransfer: %s failed with %d (%s)", operation, status, strerror(status));
    }

    return status;
}

static bool FieldCharacter(unsigned char c)
{
    return ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9')) || ((0 != c) && (NULL != strchr("!#$%&'*+-.^_`|~", c)));
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

    if (0 != (status = PlainTransfer(transport->descriptor, request, requestSize, true, deadline, log)))
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
            if (0 != (status = PlainTransfer(transport->descriptor, &byte, 1, false, deadline, log)))
            {
                return status;
            }

            if (++total > TELEMETRY_HTTP_HEADER_LIMIT)
            {
                return EMSGSIZE;
            }

            if ('\n' == byte)
            {
                if ((0 == length) || ('\r' != line[length - 1]))
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

            if (((byte < 32) && ('\t' != byte) && ('\r' != byte)) || (byte >= 127) || ((0 != length) && ('\r' == line[length - 1])))
            {
                return EPROTO;
            }

            line[length++] = (char)byte;
        }

        if (first)
        {
            if ((length < 12) || (0 != memcmp(line, "HTTP/1.", 7)) ||
                (('0' != line[7]) && ('1' != line[7])) || (' ' != line[8]) ||
                (line[9] < '1') || (line[9] > '5') || (line[10] < '0') || (line[10] > '9') ||
                (line[11] < '0') || (line[11] > '9') || ((length > 12) && (' ' != line[12])))
            {
                return EPROTO;
            }

            code = (unsigned int)((((line[9] - '0') * 100) + ((line[10] - '0') * 10) + line[11]) - '0');

            if (101 == code)
            {
                return ENOTSUP;
            }

            first = false;
        }
        else if (0 == length)
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
                // Successful CONNECT has no HTTP body, regardless of CL/TE.
                status = ((code >= 200) && (code < 300)) ? 0 : ((407 == code) ? EACCES : ECONNREFUSED);
                if (0 != status)
                {
                    OsConfigLogError(log, "Tunnel: HTTP CONNECT failed with %d (%s); HTTP status %u", status, strerror(status), code);
                }

                return status;
            }
        }
        else
        {
            if (++headers > 64)
            {
                return EMSGSIZE;
            }

            colon = strchr(line, ':');

            if ((NULL == colon) || (colon == line))
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
    const char* operation = "output validation";

    if ((NULL != transport) && (NULL != *transport))
    {
        status = EALREADY;
    }
    else if (NULL != transport)
    {
        operation = "calloc";
        created = calloc(1, sizeof(*created));
        status = (NULL != created) ? 0 : ENOMEM;

        if (0 == status)
        {
            created->descriptor = -1;
            operation = "TelemetryProxyDiscover";

            if ((0 == (status = TelemetryProxyDiscover(&selection, log))) && (TelemetryProxyConfigured == selection.kind))
            {
                created->proxied = true;
                operation = "TelemetryProxyParseHttp";
                status = TelemetryProxyParseHttp(selection.url, &created->proxy, log);
            }
        }
    }

    if (0 != status)
    {
        free(created);
        OsConfigLogError(log, "TelemetryTransportCreate: %s failed with %d (%s)", operation, status, strerror(status));
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

int TelemetryTransportSend(TelemetryTransport* transport, const char* token, const char* clientVersion, int64_t uploadTimeMilliseconds,
    const void* event, size_t eventSize, int64_t deadline, TelemetryHttpResponse* response, OsConfigLogHandle log)
{
    char headers[TELEMETRY_HTTP_HEADER_LIMIT + 1] = {0};
    size_t headerSize = 0;
    const char* stage = "argument validation";
    int status = EINVAL;
    unsigned char bytes[2048] = {0};
    size_t size = 0;
    bool eof = false;
    bool reusable = false;

    if (NULL != response)
    {
        TelemetryHttpResponseInitialize(response, log);
    }

    if ((NULL == transport) || (NULL == response) || (NULL == event))
    {
        goto failed;
    }

    if (transport->suppressed)
    {
        stage = "collector suppression check";
        status = ECANCELED;
        goto failed;
    }

    stage = "TelemetryHttpBuildRequest";
    if (0 != (status = TelemetryHttpBuildRequest(token, clientVersion, uploadTimeMilliseconds, eventSize, headers, sizeof(headers), &headerSize, log)))
    {
        goto failed;
    }

    if (transport->descriptor >= 0)
    {
        stage = "TelemetryTlsCheckIdle";

        if (0 != (status = TelemetryTlsCheckIdle(transport->tls, &reusable, deadline, log)))
        {
            goto failed;
        }

        if (!reusable)
        {
            Disconnect(transport, log);
        }
    }

    if (transport->descriptor < 0)
    {
        stage = "Connect";

        if (0 != (status = Connect(transport, deadline, log)))
        {
            goto failed;
        }

        if (transport->proxied)
        {
            stage = "Tunnel";

            if (0 != (status = Tunnel(transport, deadline, log)))
            {
                goto failed;
            }
        }

        stage = "TelemetryTlsCreate";

        if (0 == (status = TelemetryTlsCreate(&transport->tls, deadline, log)))
        {
            stage = "TelemetryTlsHandshake";
            status = TelemetryTlsHandshake(transport->tls, transport->descriptor, TELEMETRY_ARIA_HOST, deadline, log);
        }

        if (0 != status)
        {
            goto failed;
        }
    }

    stage = "TelemetryTlsWrite(headers)";

    if (0 == (status = TelemetryTlsWrite(transport->tls, headers, headerSize, deadline, log)))
    {
        stage = "TelemetryTlsWrite(event)";
        status = TelemetryTlsWrite(transport->tls, event, eventSize, deadline, log);
    }

    if (0 != status)
    {
        goto failed;
    }

    while (!response->complete)
    {
        size = 0;
        eof = false;
        stage = "TelemetryTlsRead";

        if (0 == (status = TelemetryTlsRead(transport->tls, bytes, sizeof(bytes), &size, &eof, deadline, log)))
        {
            stage = "TelemetryHttpResponseFeed";
            status = TelemetryHttpResponseFeed(response, bytes, size, eof, log);
        }

        if (0 != status)
        {
            goto failed;
        }
    }

    CaptureSuppression(transport, response);

    if (!response->reusable || transport->suppressed)
    {
        Disconnect(transport, log);
    }

    return 0;

failed:
    OsConfigLogError(log, "TelemetryTransportSend: %s failed with %d (%s); event not replayed", stage, status, strerror(status));

    if (NULL != transport)
    {
        if (NULL != response)
        {
            CaptureSuppression(transport, response);
        }

        Disconnect(transport, log);
    }

    return status;
}

bool TelemetryTransportSuppressed(const TelemetryTransport* transport)
{
    return (NULL != transport) && transport->suppressed;
}

void TelemetryTransportDestroy(TelemetryTransport** transport, OsConfigLogHandle log)
{
    if ((NULL != transport) && (NULL != *transport))
    {
        Disconnect(*transport, log);
        free(*transport);
        *transport = NULL;
    }
}
