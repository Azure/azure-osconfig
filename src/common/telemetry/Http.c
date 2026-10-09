// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "Http.h"
#include "Encoder.h"

#include <errno.h>
#include <inttypes.h>
#include <parson.h>
#include <stdio.h>
#include <string.h>

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

int TelemetryHttpBuildRequest(const char* token, const char* clientVersion, int64_t uploadTimeMilliseconds, size_t eventSize,
    char* headers, size_t capacity, size_t* headerSize, OsConfigLogHandle log)
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
