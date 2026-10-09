// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "Event.h"

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

    if (length == (capacity - *offset))
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

int TelemetryPackEvent(const char* name, const TelemetryProperty* properties, size_t count, unsigned char* payload, size_t capacity, size_t* size, OsConfigLogHandle log)
{
    int status = EINVAL;
    uint32_t propertyCount = 0;
    size_t offset = 0;
    size_t i = 0;
    const char* operation = "AppendString(event name)";

    if (NULL != size)
    {
        *size = 0;
    }

    if ((NULL == size) || (NULL == payload) || (NULL == properties) || (0 == count) || (count > TELEMETRY_MAX_PROPERTY_COUNT) ||
        (capacity < sizeof(uint32_t)) || (capacity > TELEMETRY_MAX_EVENT_SIZE))
    {
        OsConfigLogError(log, "TelemetryPackEvent: argument validation failed with %d (%s)", status, strerror(status));
        return status;
    }

    propertyCount = (uint32_t)count;
    memcpy(payload, &propertyCount, sizeof(propertyCount));
    offset = sizeof(propertyCount);
    status = AppendString(name, payload, capacity, &offset);

    for (i = 0; (0 == status) && (i < count); ++i)
    {
        if (TelemetryPropertyString != properties[i].type)
        {
            operation = "property type validation";
            status = EINVAL;
        }
        else
        {
            operation = "AppendString(property name)";
            if (0 == (status = AppendString(properties[i].name, payload, capacity, &offset)))
            {
                operation = "AppendString(property value)";
                status = AppendString(properties[i].value.stringValue, payload, capacity, &offset);
            }
        }
    }

    if (0 == status)
    {
        *size = offset;
    }
    else
    {
        OsConfigLogError(log, "TelemetryPackEvent: %s failed with %d (%s)", operation, status, strerror(status));
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

        if (length == (size - *offset))
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
    static const char* status[] = {"FileName", "LineNumber", "ScenarioName", "FunctionName", "RuleCodename", "CallingFunctionName", "Microseconds", "ResultCode", "ResultString"};
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

    if (0 == strcmp(name, "StatusTrace"))
    {
        fields = status;
        fieldCount = ARRAY_SIZE(status);
    }
    else if (0 == strcmp(name, "BaselineRun"))
    {
        fields = baseline;
        fieldCount = ARRAY_SIZE(baseline);
    }
    else if (0 == strcmp(name, "RuleComplete"))
    {
        fields = rule;
        fieldCount = ARRAY_SIZE(rule);
    }
    else if (0 == strcmp(name, "CrashDetected"))
    {
        fields = crash;
        fieldCount = ARRAY_SIZE(crash);
    }

    matches = (NULL != fields) && (count == (fieldCount + ARRAY_SIZE(common)));

    for (i = 0; matches && (i < count); ++i)
    {
        required = (i < ARRAY_SIZE(common)) ? common[i] : fields[i - ARRAY_SIZE(common)];
        occurrences = 0;

        for (j = 0; j < count; ++j)
        {
            if (0 == strcmp(required, properties[j].name))
            {
                ++occurrences;
            }
        }

        matches = 1 == occurrences;
    }

    return matches;
}

int TelemetryEncodePayload(const unsigned char* payload, size_t size, const char* iKey, const char* epoch, int64_t sequence,
    unsigned char* bytes, size_t* encodedSize, int64_t* uploadTime, OsConfigLogHandle log)
{
    int status = EINVAL;
    uint32_t count = 0;
    size_t offset = 0;
    const char* name = NULL;
    TelemetryProperty properties[TELEMETRY_MAX_PROPERTY_COUNT] = {0};
    size_t i = 0;
    struct timespec now = {0};
    TelemetryEvent event = {0};
    const char* operation = "payload framing/ReadString validation";

    if (NULL != encodedSize)
    {
        *encodedSize = 0;
    }

    if (NULL != uploadTime)
    {
        *uploadTime = 0;
    }

    if ((NULL == payload) || (size <= sizeof(uint32_t)) || (size > TELEMETRY_MAX_EVENT_SIZE) || (NULL == encodedSize) || (NULL == uploadTime) || (NULL == bytes))
    {
        OsConfigLogError(log, "TelemetryEncodePayload: argument validation failed with %d (%s)", status, strerror(status));
        return status;
    }

    memcpy(&count, payload, sizeof(count));

    if ((0 != count) && (count <= TELEMETRY_MAX_PROPERTY_COUNT))
    {
        offset = sizeof(count);
        name = ReadString(payload, size, &offset);

        if (NULL != name)
        {
            status = 0;
        }
    }

    for (i = 0; (0 == status) && (i < count); ++i)
    {
        properties[i].name = ReadString(payload, size, &offset);
        properties[i].type = TelemetryPropertyString;
        properties[i].value.stringValue = ReadString(payload, size, &offset);

        if ((NULL == properties[i].name) || (NULL == properties[i].value.stringValue))
        {
            status = EINVAL;
        }
    }

    if (0 == status)
    {
        operation = "payload framing/SchemaMatches validation";
        if ((offset != size) || !SchemaMatches(name, properties, count))
        {
            status = EINVAL;
        }
        else if (0 != clock_gettime(CLOCK_REALTIME, &now))
        {
            operation = "clock_gettime(CLOCK_REALTIME)";
            status = (0 != errno) ? errno : EIO;
        }
        else if ((now.tv_sec < 0) || ((uint64_t)now.tv_sec > UINT64_C(253402300799)))
        {
            operation = "UTC timestamp range check";
            status = EOVERFLOW;
        }
    }

    if (0 != status)
    {
        OsConfigLogError(log, "TelemetryEncodePayload: %s failed with %d (%s)", operation, status, strerror(status));
    }
    else
    {
        event.name = name;
        event.iKey = iKey;
        event.time = INT64_C(621355968000000000) + ((int64_t)now.tv_sec * 10000000) + (now.tv_nsec / 100);
        event.flags = 0x0101;
        event.sdkVersion = TELEMETRY_CLIENT_VERSION;
        event.sdkEpoch = epoch;
        event.sequence = sequence;
        event.properties = properties;
        event.propertyCount = count;

        if (0 == (status = TelemetryEncodeEvent(&event, bytes, TELEMETRY_MAX_EVENT_SIZE, encodedSize, log)))
        {
            *uploadTime = ((int64_t)now.tv_sec * 1000) + (now.tv_nsec / 1000000);
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
    int closeStatus = 0;
    const char* operation = "open(/dev/urandom)";

    if (NULL == epoch)
    {
        status = EINVAL;
        OsConfigLogError(log, "TelemetryCreateEpoch: epoch pointer validation failed with %d (%s)", status, strerror(status));
        return status;
    }

    descriptor = open("/dev/urandom", O_RDONLY | O_CLOEXEC);

    if (descriptor < 0)
    {
        status = (0 != errno) ? errno : EIO;
    }

    while ((0 == status) && (size < sizeof(bytes)))
    {
        operation = "read(/dev/urandom)";
        count = read(descriptor, bytes + size, sizeof(bytes) - size);

        if (count > 0)
        {
            size += (size_t)count;
        }
        else if (0 == count)
        {
            status = EIO;
        }
        else if (EINTR != errno)
        {
            status = (0 != errno) ? errno : EIO;
        }
    }

    if ((descriptor >= 0) && (0 != close(descriptor)))
    {
        closeStatus = (0 != errno) ? errno : EIO;
        OsConfigLogError(log, "TelemetryCreateEpoch: close(/dev/urandom) failed with %d (%s)", closeStatus, strerror(closeStatus));

        if (0 == status)
        {
            operation = "close(/dev/urandom)";
            status = closeStatus;
        }
    }

    if (0 != status)
    {
        OsConfigLogError(log, "TelemetryCreateEpoch: %s failed with %d (%s)", operation, status, strerror(status));
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
