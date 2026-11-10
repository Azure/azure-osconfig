// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef EVENT_PAYLOAD_H
#define EVENT_PAYLOAD_H

#include <Logging.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TELEMETRY_MAX_EVENT_SIZE 16384
#define TELEMETRY_MAX_PROPERTY_COUNT 32

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum TelemetryPropertyType
{
    TelemetryPropertyString,
    TelemetryPropertyInt64,
    TelemetryPropertyDouble,
    TelemetryPropertyBoolean
} TelemetryPropertyType;

typedef struct TelemetryProperty
{
    const char* name;
    TelemetryPropertyType type;
    union
    {
        const char* stringValue;
        int64_t int64Value;
        double doubleValue;
        bool booleanValue;
    } value;
} TelemetryProperty;

// IPC v1: native uint32 count, then NUL-terminated event name and count
// name/value string pairs. This is not the collector's event encoding.
int TelemetryPackEvent(const char* name, const TelemetryProperty* properties, size_t count,
    unsigned char* payload, size_t capacity, size_t* size, OsConfigLogHandle log);

#ifdef __cplusplus
}
#endif

#endif // EVENT_PAYLOAD_H
