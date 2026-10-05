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
    size_t length = 0;
    int status = 0;

    if (NULL == value)
    {
        return EINVAL;
    }
    length = strnlen(value, capacity - *offset);
    if (length == capacity - *offset)
    {
        status = EMSGSIZE;
    }
    else
    {
        memcpy(payload + *offset, value, length + 1);
        *offset += length + 1;
    }
    return status;
}

int TelemetryPackEvent(const char* name, const TelemetryProperty* properties, size_t count,
    unsigned char* payload, size_t capacity, size_t* size, OsConfigLogHandle log)
{
    int status = EINVAL;
    uint32_t propertyCount = 0;
    size_t offset = 0;
    size_t i = 0;

    if (size)
    {
        *size = 0;
    }
    if ((!size) || (!payload) || (!properties) || (!count) || (count > TELEMETRY_MAX_PROPERTY_COUNT) ||
        (capacity < sizeof(uint32_t)) || (capacity > TELEMETRY_MAX_EVENT_SIZE))
    {
        OsConfigLogError(log, "TelemetryPackEvent: Invalid or oversized event (status=%d)", status);
        return status;
    }
    propertyCount = (uint32_t)count;
    memcpy(payload, &propertyCount, sizeof(propertyCount));
    offset = sizeof(propertyCount);
    status = AppendString(name, payload, capacity, &offset);
    for (i = 0; (!status) && (i < count); ++i)
    {
        if (TelemetryPropertyString != properties[i].type)
        {
            status = EINVAL;
        }
        else
        {
            status = AppendString(properties[i].name, payload, capacity, &offset);
            if (!status)
            {
                status = AppendString(properties[i].value.stringValue, payload, capacity, &offset);
            }
        }
    }
    if (!status)
    {
        *size = offset;
    }
    else
    {
        OsConfigLogError(log, "TelemetryPackEvent: Invalid or oversized event (status=%d)", status);
    }
    return status;
}

static const char* ReadString(const unsigned char* payload, size_t size, size_t* offset)
{
    const char* value = NULL;
    size_t length = 0;

    if (*offset < size)
    {
        value = (const char*)payload + *offset;
        length = strnlen(value, size - *offset);
        if (length == size - *offset)
        {
            value = NULL;
        }
        else
        {
            *offset += length + 1;
        }
    }
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
    size_t i = 0;
    const char* required = NULL;
    size_t occurrences = 0;
    size_t j = 0;
    bool matches = false;

    if (!strcmp(name, "StatusTrace"))
    {
        fields = status;
        fieldCount = ARRAY_SIZE(status);
    }
    else if (!strcmp(name, "BaselineRun"))
    {
        fields = baseline;
        fieldCount = ARRAY_SIZE(baseline);
    }
    else if (!strcmp(name, "RuleComplete"))
    {
        fields = rule;
        fieldCount = ARRAY_SIZE(rule);
    }
    else if (!strcmp(name, "CrashDetected"))
    {
        fields = crash;
        fieldCount = ARRAY_SIZE(crash);
    }
    matches = (fields) && (count == fieldCount + ARRAY_SIZE(common));
    for (i = 0; (matches) && (i < count); ++i)
    {
        required = i < ARRAY_SIZE(common) ? common[i] : fields[i - ARRAY_SIZE(common)];
        occurrences = 0;
        for (j = 0; j < count; ++j)
        {
            if (!strcmp(required, properties[j].name))
            {
                ++occurrences;
            }
        }
        matches = 1 == occurrences;
    }
    return matches;
}

int TelemetryEncodePayload(const unsigned char* payload, size_t size, const char* iKey,
    const char* epoch, int64_t sequence, unsigned char* bytes, size_t* encodedSize,
    int64_t* uploadTime, OsConfigLogHandle log)
{
    int status = EINVAL;
    uint32_t count = 0;
    size_t offset = 0;
    const char* name = NULL;
    TelemetryProperty properties[TELEMETRY_MAX_PROPERTY_COUNT] = {0};
    size_t i = 0;
    struct timespec now = {0};
    TelemetryEvent event = {0};

    if (encodedSize)
    {
        *encodedSize = 0;
    }
    if (uploadTime)
    {
        *uploadTime = 0;
    }
    if ((!payload) || (size <= sizeof(uint32_t)) || (size > TELEMETRY_MAX_EVENT_SIZE) ||
        (!encodedSize) || (!uploadTime) || (!bytes))
    {
        OsConfigLogError(log, "TelemetryEncodePayload: Invalid event or clock (status=%d)", status);
        return status;
    }
    memcpy(&count, payload, sizeof(count));
    if ((count) && (count <= TELEMETRY_MAX_PROPERTY_COUNT))
    {
        offset = sizeof(count);
        name = ReadString(payload, size, &offset);
        if (name)
        {
            status = 0;
        }
    }
    for (i = 0; (!status) && (i < count); ++i)
    {
        properties[i].name = ReadString(payload, size, &offset);
        properties[i].type = TelemetryPropertyString;
        properties[i].value.stringValue = ReadString(payload, size, &offset);
        if ((!properties[i].name) || (!properties[i].value.stringValue))
        {
            status = EINVAL;
        }
    }
    if (!status)
    {
        if ((offset != size) || (!SchemaMatches(name, properties, count)))
        {
            status = EINVAL;
        }
        else if (clock_gettime(CLOCK_REALTIME, &now))
        {
            status = errno ? errno : EIO;
        }
        else if ((now.tv_sec < 0) || ((uint64_t)now.tv_sec > UINT64_C(253402300799)))
        {
            status = EOVERFLOW;
        }
    }
    if (status)
    {
        OsConfigLogError(log, "TelemetryEncodePayload: Invalid event or clock (status=%d)", status);
    }
    else
    {
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
        if (!status)
        {
            *uploadTime = (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
        }
    }
    return status;
}

int TelemetryCreateEpoch(char* epoch, OsConfigLogHandle log)
{
    unsigned char bytes[16] = {0};
    int status = 0;
    int descriptor = 0;
    size_t size = 0;
    ssize_t count = 0;
    size_t offset = 0;
    static const char hex[] = "0123456789abcdef";
    size_t i = 0;

    if (!epoch)
    {
        status = EINVAL;
        OsConfigLogError(log, "TelemetryCreateEpoch: Random ID failed (status=%d)", status);
        return status;
    }
    descriptor = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (descriptor < 0)
    {
        status = errno ? errno : EIO;
    }
    while ((!status) && (size < sizeof(bytes)))
    {
        count = read(descriptor, bytes + size, sizeof(bytes) - size);
        if (count > 0)
        {
            size += (size_t)count;
        }
        else if (!count)
        {
            status = EIO;
        }
        else if (EINTR != errno)
        {
            status = errno ? errno : EIO;
        }
    }
    if ((descriptor >= 0) && (close(descriptor)) && (!status))
    {
        status = errno ? errno : EIO;
    }
    if (status)
    {
        OsConfigLogError(log, "TelemetryCreateEpoch: Random ID failed (status=%d)", status);
    }
    else
    {
        bytes[6] = (bytes[6] & 15) | 64;
        bytes[8] = (bytes[8] & 63) | 128;
        for (i = 0; i < sizeof(bytes); ++i)
        {
            if ((4 == i) || (6 == i) || (8 == i) || (10 == i))
            {
                epoch[offset++] = '-';
            }
            epoch[offset++] = hex[bytes[i] >> 4];
            epoch[offset++] = hex[bytes[i] & 15];
        }
        epoch[offset] = '\0';
    }
    return status;
}
