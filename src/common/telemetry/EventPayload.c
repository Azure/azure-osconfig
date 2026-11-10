// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _POSIX_C_SOURCE 200809L

#include "EventPayload.h"

#include <errno.h>
#include <string.h>

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

int TelemetryPackEvent(const char* name, const TelemetryProperty* properties, size_t count,
    unsigned char* payload, size_t capacity, size_t* size, OsConfigLogHandle log)
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

    if ((NULL == size) || (NULL == payload) || (NULL == properties) || (0 == count) ||
        (count > TELEMETRY_MAX_PROPERTY_COUNT) || (capacity < sizeof(uint32_t)) || (capacity > TELEMETRY_MAX_EVENT_SIZE))
    {
        OsConfigLogInfo(log, "TelemetryPackEvent: argument validation failed with %d (%s)", status, strerror(status));
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
        OsConfigLogInfo(log, "TelemetryPackEvent: %s failed with %d (%s)", operation, status, strerror(status));
    }

    return status;
}
