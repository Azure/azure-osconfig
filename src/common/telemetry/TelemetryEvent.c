// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "TelemetryEvent.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int AppendString(const char* value, unsigned char* payload, size_t capacity, size_t* offset)
{
    if (NULL == value) return EINVAL;
    size_t length = strnlen(value, capacity - *offset);
    if (length == capacity - *offset) return EMSGSIZE;
    memcpy(payload + *offset, value, length + 1);
    *offset += length + 1;
    return 0;
}

int TelemetryPackEvent(const char* name, const TelemetryProperty* properties, size_t count,
    unsigned char* payload, size_t capacity, size_t* size, OsConfigLogHandle log)
{
    int status = EINVAL;
    if (size) *size = 0;
    if (!size || !payload || !properties || !count || count > TELEMETRY_MAX_PROPERTY_COUNT ||
        capacity < sizeof(uint32_t) || capacity > TELEMETRY_MAX_EVENT_SIZE) goto failed;
    uint32_t propertyCount = (uint32_t)count;
    memcpy(payload, &propertyCount, sizeof(propertyCount));
    size_t offset = sizeof(propertyCount);
    status = AppendString(name, payload, capacity, &offset);
    for (size_t i = 0; !status && i < count; ++i)
    {
        if (properties[i].type != TelemetryPropertyString) { status = EINVAL; break; }
        status = AppendString(properties[i].name, payload, capacity, &offset);
        if (!status) status = AppendString(properties[i].value.stringValue, payload, capacity, &offset);
    }
    if (!status)
    {
        *size = offset;
        return 0;
    }
failed:
    OsConfigLogError(log, "TelemetryPackEvent: Invalid or oversized event (status=%d)", status);
    return status;
}

static const char* ReadString(const unsigned char* payload, size_t size, size_t* offset)
{
    if (*offset >= size) return NULL;
    const char* value = (const char*)payload + *offset;
    size_t length = strnlen(value, size - *offset);
    if (length == size - *offset) return NULL;
    *offset += length + 1;
    return value;
}

static bool SchemaMatches(const char* name, const TelemetryProperty* properties, size_t count)
{
    static const char* common[] = {"DistroName", "CorrelationId", "Version", "Timestamp"};
    static const char* status[] = {"FileName", "LineNumber", "ScenarioName", "FunctionName",
        "RuleCodename", "CallingFunctionName", "Microseconds", "ResultCode", "ResultString"};
    static const char* baseline[] = {"BaselineName", "Mode", "DurationSeconds"};
    static const char* rule[] = {"ComponentName", "ObjectName", "ObjectResult", "Microseconds"};
    static const char* crash[] = {"CrashInfo"};
    const char** fields = NULL;
    size_t fieldCount = 0;
    if (!strcmp(name, "StatusTrace")) { fields = status; fieldCount = ARRAY_SIZE(status); }
    else if (!strcmp(name, "BaselineRun")) { fields = baseline; fieldCount = ARRAY_SIZE(baseline); }
    else if (!strcmp(name, "RuleComplete")) { fields = rule; fieldCount = ARRAY_SIZE(rule); }
    else if (!strcmp(name, "CrashDetected")) { fields = crash; fieldCount = ARRAY_SIZE(crash); }
    else return false;
    if (count != fieldCount + ARRAY_SIZE(common)) return false;
    for (size_t i = 0; i < count; ++i)
    {
        const char* required = i < ARRAY_SIZE(common) ? common[i] : fields[i - ARRAY_SIZE(common)];
        size_t occurrences = 0;
        for (size_t j = 0; j < count; ++j)
            if (!strcmp(required, properties[j].name)) ++occurrences;
        if (occurrences != 1) return false;
    }
    return true;
}

int TelemetryEncodePayload(const unsigned char* payload, size_t size, const char* iKey,
    const char* epoch, int64_t sequence, unsigned char* bytes, size_t* encodedSize,
    int64_t* uploadTime, OsConfigLogHandle log)
{
    int status = EINVAL;
    if (encodedSize) *encodedSize = 0;
    if (uploadTime) *uploadTime = 0;
    if (!payload || size <= sizeof(uint32_t) || size > TELEMETRY_MAX_EVENT_SIZE ||
        !encodedSize || !uploadTime || !bytes) goto failed;
    uint32_t count = 0;
    memcpy(&count, payload, sizeof(count));
    if (!count || count > TELEMETRY_MAX_PROPERTY_COUNT) goto failed;
    size_t offset = sizeof(count);
    const char* name = ReadString(payload, size, &offset);
    if (!name) goto failed;
    TelemetryProperty properties[TELEMETRY_MAX_PROPERTY_COUNT] = {0};
    for (size_t i = 0; i < count; ++i)
    {
        properties[i].name = ReadString(payload, size, &offset);
        properties[i].type = TelemetryPropertyString;
        properties[i].value.stringValue = ReadString(payload, size, &offset);
        if (!properties[i].name || !properties[i].value.stringValue) goto failed;
    }
    if (offset != size || !SchemaMatches(name, properties, count)) goto failed;
    struct timespec now = {0};
    if (clock_gettime(CLOCK_REALTIME, &now)) { status = errno ? errno : EIO; goto failed; }
    if (now.tv_sec < 0 || (uint64_t)now.tv_sec > UINT64_C(253402300799))
    {
        status = EOVERFLOW;
        goto failed;
    }
    TelemetryEvent event = {0};
    event.name = name;
    event.iKey = iKey;
    event.time = INT64_C(621355968000000000) + (int64_t)now.tv_sec * 10000000 + now.tv_nsec / 100;
    event.flags = 0x0101;
    event.sdkVersion = TELEMETRY_CLIENT_VERSION;
    event.sdkEpoch = epoch;
    event.sequence = sequence;
    event.properties = properties;
    event.propertyCount = count;
    status = TelemetryEncodeEvent(&event, bytes, TELEMETRY_MAX_EVENT_SIZE, encodedSize, log);
    if (status) return status;
    *uploadTime = (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
    return 0;
failed:
    OsConfigLogError(log, "TelemetryEncodePayload: Invalid event or clock (status=%d)", status);
    return status;
}

int TelemetryCreateEpoch(char* epoch, OsConfigLogHandle log)
{
    unsigned char bytes[16];
    int status = 0;
    if (!epoch) { status = EINVAL; goto failed; }
    int descriptor = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (descriptor < 0) { status = errno ? errno : EIO; goto failed; }
    size_t size = 0;
    while (size < sizeof(bytes))
    {
        ssize_t count = read(descriptor, bytes + size, sizeof(bytes) - size);
        if (count > 0) size += (size_t)count;
        else if (!count) { status = EIO; break; }
        else if (errno != EINTR) { status = errno ? errno : EIO; break; }
    }
    if (close(descriptor) && !status) status = errno ? errno : EIO;
    if (status) goto failed;
    bytes[6] = (bytes[6] & 15) | 64;
    bytes[8] = (bytes[8] & 63) | 128;
    size_t offset = 0;
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < sizeof(bytes); ++i)
    {
        if (i == 4 || i == 6 || i == 8 || i == 10) epoch[offset++] = '-';
        epoch[offset++] = hex[bytes[i] >> 4];
        epoch[offset++] = hex[bytes[i] & 15];
    }
    epoch[offset] = '\0';
    return 0;
failed:
    OsConfigLogError(log, "TelemetryCreateEpoch: Random ID failed (status=%d)", status);
    return status;
}
