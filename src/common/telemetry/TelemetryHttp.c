// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "TelemetryHttp.h"
#include "TelemetryEncoder.h"

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
    for (; *left && *right; ++left, ++right)
    {
        unsigned char a = (unsigned char)*left;
        unsigned char b = (unsigned char)*right;
        if ((a >= 'A') && (a <= 'Z')) a += 'a' - 'A';
        if ((b >= 'A') && (b <= 'Z')) b += 'a' - 'A';
        if (a != b) return false;
    }
    return *left == *right;
}

static bool TokenCharacter(unsigned char value)
{
    return ((value >= '0') && (value <= '9')) ||
        ((value >= 'a') && (value <= 'z')) || ((value >= 'A') && (value <= 'Z')) ||
        ((value != 0) && (NULL != strchr("!#$%&'*+-.^_`|~", value)));
}

static char* Trim(char* value)
{
    while ((*value == ' ') || (*value == '\t')) ++value;
    size_t length = strlen(value);
    while (length && ((value[length - 1] == ' ') || (value[length - 1] == '\t')))
    {
        value[--length] = '\0';
    }
    return value;
}

static bool HeaderValue(const char* value, size_t limit)
{
    if ((NULL == value) || ('\0' == *value)) return false;
    size_t size = strnlen(value, limit + 1);
    if (size > limit) return false;
    for (size_t i = 0; i < size; ++i)
    {
        if (((unsigned char)value[i] <= 32) || ((unsigned char)value[i] >= 127)) return false;
    }
    return true;
}

int TelemetryHttpBuildRequest(const char* token, const char* clientVersion,
    int64_t uploadTimeMilliseconds, size_t eventSize, char* headers,
    size_t capacity, size_t* headerSize, OsConfigLogHandle log)
{
    char formatted[TELEMETRY_HTTP_HEADER_LIMIT + 1];
    int status = 0;
    if (NULL != headerSize) *headerSize = 0;
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
        int length = snprintf(formatted, sizeof(formatted),
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
        OsConfigLogInfo(log, "TelemetryHttpBuildRequest: Failed (status=%d)", status);
    }
    return status;
}

static int Number(const char* text, size_t length, unsigned int base, size_t* value)
{
    if (0 == length) return EPROTO;
    size_t number = 0;
    for (size_t i = 0; i < length; ++i)
    {
        unsigned int digit;
        unsigned char c = (unsigned char)text[i];
        if ((c >= '0') && (c <= '9')) digit = c - '0';
        else if ((16 == base) && (c >= 'a') && (c <= 'f')) digit = c - 'a' + 10;
        else if ((16 == base) && (c >= 'A') && (c <= 'F')) digit = c - 'A' + 10;
        else return EPROTO;
        if ((digit >= base) || (number > (SIZE_MAX - digit) / base)) return EOVERFLOW;
        number = number * base + digit;
    }
    *value = number;
    return 0;
}

static int Connection(TelemetryHttpResponse* response, char* value)
{
    for (;;)
    {
        char* comma = strchr(value, ',');
        if (NULL != comma) *comma = '\0';
        char* token = Trim(value);
        if ('\0' == *token) return EPROTO;
        for (const char* c = token; *c; ++c)
        {
            if (!TokenCharacter((unsigned char)*c)) return EPROTO;
        }
        if (Equal(token, "close")) response->connectionClose = true;
        if (Equal(token, "keep-alive")) response->connectionKeepAlive = true;
        if (NULL == comma) return 0;
        value = comma + 1;
    }
}

static int SaveControl(TelemetryHttpResponse* response, TelemetryHttpControlKind kind, const char* value)
{
    size_t length = strlen(value) + 1;
    if ((response->controlCount >= TELEMETRY_HTTP_CONTROL_LIMIT) ||
        (length > sizeof(response->controlValues) - response->controlBytes))
    {
        return EMSGSIZE;
    }
    TelemetryHttpControl* control = &response->controls[response->controlCount++];
    control->kind = kind;
    control->offset = response->controlBytes;
    memcpy(response->controlValues + response->controlBytes, value, length);
    response->controlBytes += length;
    return 0;
}

static int Header(TelemetryHttpResponse* response, bool trailer)
{
    if (++response->headerCount > 64) return EMSGSIZE;
    char* colon = strchr(response->line, ':');
    if ((NULL == colon) || (colon == response->line)) return EPROTO;
    for (char* c = response->line; c < colon; ++c)
    {
        if (!TokenCharacter((unsigned char)*c)) return EPROTO;
    }
    *colon = '\0';
    char* value = Trim(colon + 1);
    for (const char* c = value; *c; ++c)
    {
        if (((unsigned char)*c < 32 && *c != '\t') || ((unsigned char)*c >= 127)) return EPROTO;
    }
    const char* name = response->line;
    bool length = Equal(name, "Content-Length");
    bool transfer = Equal(name, "Transfer-Encoding");
    bool connection = Equal(name, "Connection");
    bool encoding = Equal(name, "Content-Encoding");
    bool retry = Equal(name, "Retry-After");
    bool kills = Equal(name, "kill-tokens");
    bool duration = Equal(name, "kill-duration");
    bool delta = Equal(name, "time-delta-millis");
    if (trailer)
    {
        // Trailers cannot replace framing, decoding, or collector control decisions.
        return (length || transfer || connection || encoding || retry || kills || duration || delta) ? EPROTO : 0;
    }
    if (length)
    {
        size_t parsed = 0;
        int status = Number(value, strlen(value), 10, &parsed);
        if (0 != status) return status;
        if (response->hasLength && (response->contentLength != parsed)) return EPROTO;
        response->contentLength = parsed;
        response->hasLength = true;
    }
    else if (transfer)
    {
        if (response->chunked) return EPROTO;
        if (!Equal(value, "chunked")) return ENOTSUP;
        response->chunked = true;
    }
    else if (connection)
    {
        return Connection(response, value);
    }
    else if (encoding)
    {
        if (!Equal(value, "identity")) return ENOTSUP;
    }
    else if (response->status >= 200)
    {
        if (retry) return SaveControl(response, TelemetryRetryAfter, value);
        if (kills) return SaveControl(response, TelemetryKillTokens, value);
        if (duration) return SaveControl(response, TelemetryKillDuration, value);
        if (delta) return SaveControl(response, TelemetryTimeDeltaMillis, value);
    }
    return 0;
}

static bool JsonWhitespace(char c)
{
    return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n');
}

static int JsonEnvelope(const char* body, size_t size)
{
    // Parson parses the first value, so bound nesting and require its complete
    // object envelope before calling it. Parson still validates JSON syntax.
    char stack[16];
    size_t depth = 0;
    bool quoted = false;
    bool escaped = false;
    bool started = false;
    bool ended = false;
    for (size_t i = 0; i < size; ++i)
    {
        char c = body[i];
        if ('\0' == c) return EPROTO;
        if (!quoted && (unsigned char)c < 32 && !JsonWhitespace(c)) return EPROTO;
        if (ended || !started)
        {
            if (JsonWhitespace(c)) continue;
            if (ended || c != '{') return EPROTO;
            started = true;
        }
        if (quoted)
        {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') quoted = false;
        }
        else if (c == '"') quoted = true;
        else if ((c == '{') || (c == '['))
        {
            if (depth == sizeof(stack)) return EMSGSIZE;
            stack[depth++] = c;
        }
        else if ((c == '}') || (c == ']'))
        {
            if (!depth || (stack[--depth] != ((c == '}') ? '{' : '['))) return EPROTO;
            if (!depth) ended = true;
        }
    }
    return (ended && !quoted) ? 0 : EPROTO;
}

static int EventFailures(const JSON_Object* object, bool* rejected)
{
    JSON_Value* failures = json_object_get_value(object, "efi");
    if (NULL == failures) return 0;
    JSON_Object* entries = json_value_get_object(failures);
    if (NULL == entries) return EPROTO;
    for (size_t i = 0; i < json_object_get_count(entries); ++i)
    {
        JSON_Value* entry = json_object_get_value_at(entries, i);
        if (JSONString == json_value_get_type(entry))
        {
            const char* value = json_value_get_string(entry);
            if ((3 != json_value_get_string_len(entry)) || (0 != strcmp(value, "all"))) return EPROTO;
            *rejected = true;
        }
        else if (JSONArray == json_value_get_type(entry))
        {
            JSON_Array* indices = json_value_get_array(entry);
            for (size_t j = 0; j < json_array_get_count(indices); ++j)
            {
                JSON_Value* index = json_array_get_value(indices, j);
                if ((JSONNumber != json_value_get_type(index)) || (0 != json_value_get_number(index)))
                {
                    return EPROTO;
                }
                *rejected = true;
            }
        }
        else return EPROTO;
    }
    return 0;
}

static const char* JsonType(const JSON_Value* value)
{
    if (NULL == value) return "missing";
    switch (json_value_get_type(value))
    {
        case JSONNull: return "null";
        case JSONString: return "string";
        case JSONNumber: return "number";
        case JSONObject: return "object";
        case JSONArray: return "array";
        case JSONBoolean: return "boolean";
        default: return "invalid";
    }
}

static int Acknowledge(TelemetryHttpResponse* response, OsConfigLogHandle log)
{
    if (200 != response->status)
    {
        response->acceptance = TelemetryRejected;
        return 0;
    }
    if (0 == response->bodySize)
    {
        response->acceptance = TelemetryUnconfirmed;
        return 0;
    }
    int status = JsonEnvelope(response->body, response->bodySize);
    if (0 != status)
    {
        OsConfigLogInfo(log, "TelemetryHttp: Acknowledgment envelope invalid (status=%d, bodyBytes=%zu)",
            status, response->bodySize);
        return status;
    }
    // Parson's string constructor validates UTF-8; its JSON parser does not.
    JSON_Value* utf8 = json_value_init_string_with_len(response->body, response->bodySize);
    if (NULL == utf8)
    {
        OsConfigLogInfo(log, "TelemetryHttp: Acknowledgment UTF-8 validation or allocation failed");
        return EPROTO;
    }
    json_value_free(utf8);
    JSON_Value* value = json_parse_string(response->body);
    if (NULL == value)
    {
        OsConfigLogInfo(log, "TelemetryHttp: Acknowledgment JSON parse or allocation failed");
        return EPROTO;
    }
    JSON_Object* object = json_value_get_object(value);
    JSON_Value* accepted = json_object_get_value(object, "acc");
    JSON_Value* rejected = json_object_get_value(object, "rej");
    double acc = json_value_get_number(accepted);
    double rej = json_value_get_number(rejected);
    bool failed = NULL != json_object_get_value(object, "TokenCrackingFailure");
    if ((JSONNumber != json_value_get_type(accepted)) || (JSONNumber != json_value_get_type(rejected)) ||
        !((acc == 0 && rej == 1) || (acc == 1 && rej == 0)))
    {
        status = EPROTO;
        OsConfigLogInfo(log, "TelemetryHttp: Acknowledgment counts invalid "
            "(accType=%s, acc=%.17g, rejType=%s, rej=%.17g, efiType=%s, tokenFailure=%d)",
            JsonType(accepted), acc, JsonType(rejected), rej,
            JsonType(json_object_get_value(object, "efi")), (int)failed);
    }
    else if (0 == (status = EventFailures(object, &failed)))
    {
        response->acceptance = (failed || rej == 1) ? TelemetryRejected : TelemetryAccepted;
    }
    else
    {
        OsConfigLogInfo(log, "TelemetryHttp: Acknowledgment event failures invalid (efiType=%s, status=%d)",
            JsonType(json_object_get_value(object, "efi")), status);
    }
    json_value_free(value);
    return status;
}

static int Complete(TelemetryHttpResponse* response, OsConfigLogHandle log)
{
    response->body[response->bodySize] = '\0';
    int status = Acknowledge(response, log);
    if (0 != status) return status;
    response->complete = true;
    response->state = HttpComplete;
    response->reusable = !response->connectionClose &&
        ((response->minorVersion == 1) || response->connectionKeepAlive);
    return 0;
}

static int EndHeaders(TelemetryHttpResponse* response, OsConfigLogHandle log)
{
    if (response->hasLength && response->chunked) return EPROTO;
    if (response->status < 200)
    {
        if (101 == response->status) return ENOTSUP;
        if (response->hasLength || response->chunked) return EPROTO;
        if (++response->informationalCount > 4) return EMSGSIZE;
        response->state = HttpStatus;
        response->connectionClose = false;
        response->connectionKeepAlive = false;
        return 0;
    }
    if (204 == response->status)
    {
        if (response->hasLength || response->chunked) return EPROTO;
        return Complete(response, log);
    }
    if (304 == response->status) return Complete(response, log);
    if (response->chunked)
    {
        if (response->minorVersion != 1) return EPROTO;
        response->state = HttpChunkSize;
    }
    else if (response->hasLength)
    {
        if (response->contentLength > TELEMETRY_HTTP_BODY_LIMIT) return EMSGSIZE;
        response->remaining = response->contentLength;
        response->state = HttpLengthBody;
        if (!response->remaining) return Complete(response, log);
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
    while ((*end >= '0' && *end <= '9') || (*end >= 'a' && *end <= 'f') || (*end >= 'A' && *end <= 'F'))
    {
        ++end;
    }
    int status = Number(response->line, (size_t)(end - response->line), 16, &response->remaining);
    if (0 != status) return status;
    // Accept bounded RFC chunk extensions, including quoted values/escapes.
    while (*end)
    {
        while (*end == ' ' || *end == '\t') ++end;
        if (*end++ != ';') return EPROTO;
        while (*end == ' ' || *end == '\t') ++end;
        if (!TokenCharacter((unsigned char)*end)) return EPROTO;
        while (TokenCharacter((unsigned char)*end)) ++end;
        while (*end == ' ' || *end == '\t') ++end;
        if (*end == '=')
        {
            ++end;
            while (*end == ' ' || *end == '\t') ++end;
            if (*end == '"')
            {
                ++end;
                while (*end && *end != '"')
                {
                    if (*end == '\\') ++end;
                    if (((unsigned char)*end < 32 && *end != '\t') || (unsigned char)*end >= 127) return EPROTO;
                    ++end;
                }
                if (*end++ != '"') return EPROTO;
            }
            else
            {
                if (!TokenCharacter((unsigned char)*end)) return EPROTO;
                while (TokenCharacter((unsigned char)*end)) ++end;
            }
        }
    }
    if (response->remaining > TELEMETRY_HTTP_BODY_LIMIT - response->bodySize) return EMSGSIZE;
    response->state = response->remaining ? HttpChunkBody : HttpTrailers;
    return 0;
}

static int Line(TelemetryHttpResponse* response, OsConfigLogHandle log)
{
    switch (response->state)
    {
        case HttpStatus:
            if ((response->lineSize < 13) || (0 != memcmp(response->line, "HTTP/1.", 7)) ||
                ((response->line[7] != '0') && (response->line[7] != '1')) ||
                (response->line[8] != ' ') || (response->line[9] < '1') ||
                (response->line[9] > '5') || (response->line[10] < '0') ||
                (response->line[10] > '9') || (response->line[11] < '0') ||
                (response->line[11] > '9') || (response->line[12] != ' '))
            {
                return EPROTO;
            }
            for (size_t i = 13; i < response->lineSize; ++i)
            {
                unsigned char c = (unsigned char)response->line[i];
                if ((c < 32 && c != '\t') || c >= 127) return EPROTO;
            }
            response->minorVersion = response->line[7] - '0';
            response->status = (unsigned int)(response->line[9] - '0') * 100 +
                (unsigned int)(response->line[10] - '0') * 10 + (unsigned int)(response->line[11] - '0');
            response->state = HttpHeaders;
            return 0;
        case HttpHeaders:
            return response->lineSize ? Header(response, false) : EndHeaders(response, log);
        case HttpChunkSize:
            return ChunkSize(response);
        case HttpTrailers:
            return response->lineSize ? Header(response, true) : Complete(response, log);
        default:
            return EPROTO;
    }
}

static int Byte(TelemetryHttpResponse* response, unsigned char byte, OsConfigLogHandle log)
{
    if (++response->wireBytes > TELEMETRY_HTTP_WIRE_LIMIT) return EMSGSIZE;
    switch (response->state)
    {
        case HttpStatus:
        case HttpHeaders:
        case HttpChunkSize:
        case HttpTrailers:
            if ((response->state != HttpChunkSize) && (++response->headerBytes > TELEMETRY_HTTP_HEADER_LIMIT))
            {
                return EMSGSIZE;
            }
            if (response->carriageReturn)
            {
                if (byte != '\n') return EPROTO;
                response->carriageReturn = false;
                response->line[response->lineSize] = '\0';
                int status = Line(response, log);
                response->lineSize = 0;
                return status;
            }
            if ((byte == 0) || (byte == '\n')) return EPROTO;
            if (byte == '\r')
            {
                response->carriageReturn = true;
                return 0;
            }
            if (response->lineSize == TELEMETRY_HTTP_LINE_LIMIT) return EMSGSIZE;
            response->line[response->lineSize++] = (char)byte;
            return 0;
        case HttpLengthBody:
        case HttpEofBody:
        case HttpChunkBody:
            if (response->bodySize == TELEMETRY_HTTP_BODY_LIMIT) return EMSGSIZE;
            response->body[response->bodySize++] = (char)byte;
            if (response->state != HttpEofBody)
            {
                if (0 == --response->remaining)
                {
                    if (response->state == HttpLengthBody) return Complete(response, log);
                    response->state = HttpChunkCr;
                }
            }
            return 0;
        case HttpChunkCr:
            if (byte != '\r') return EPROTO;
            response->state = HttpChunkLf;
            return 0;
        case HttpChunkLf:
            if (byte != '\n') return EPROTO;
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
        OsConfigLogInfo(log, "TelemetryHttpResponseInitialize: Invalid response pointer");
        return EINVAL;
    }
    memset(response, 0, sizeof(*response));
    response->state = HttpStatus;
    return 0;
}

int TelemetryHttpResponseFeed(TelemetryHttpResponse* response, const void* bytes,
    size_t size, bool endOfStream, OsConfigLogHandle log)
{
    int status = 0;
    if ((NULL == response) || ((NULL == bytes) && (0 != size)) ||
        ((NULL != response) && (response->state == HttpFailed)))
    {
        status = EINVAL;
    }
    else
    {
        const unsigned char* input = bytes;
        for (size_t i = 0; i < size && !status; ++i)
        {
            status = Byte(response, input[i], log);
        }
        if (!status && endOfStream)
        {
            if (response->state == HttpEofBody) status = Complete(response, log);
            else if (!response->complete) status = EPROTO;
            response->reusable = false;
        }
    }
    if (0 != status)
    {
        if (NULL != response)
        {
            OsConfigLogInfo(log, "TelemetryHttp: Response failure "
                "(state=%u, http=%u, bodyBytes=%zu, wireBytes=%zu, hasLength=%d, contentLength=%zu, "
                "chunked=%d, remaining=%zu, eof=%d, controls=%zu)",
                response->state, response->status, response->bodySize, response->wireBytes,
                (int)response->hasLength, response->contentLength, (int)response->chunked,
                response->remaining, (int)endOfStream, response->controlCount);
            for (size_t i = 0; i < response->controlCount; ++i)
            {
                OsConfigLogInfo(log, "TelemetryHttp: Preserved control (index=%zu, kind=%d)",
                    i, (int)response->controls[i].kind);
            }
            response->state = HttpFailed;
            response->complete = false;
            response->reusable = false;
            response->acceptance = TelemetryUnconfirmed;
        }
        OsConfigLogInfo(log, "TelemetryHttpResponseFeed: Invalid or unsupported response (status=%d)", status);
    }
    return status;
}
