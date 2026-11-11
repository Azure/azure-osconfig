// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef EVENT_PAYLOAD_H
#define EVENT_PAYLOAD_H

#include <Logging.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TELEMETRY_MAX_EVENT_SIZE 16384
#define TELEMETRY_MAX_PROPERTY_COUNT 100

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

// Pipe v3 JSON envelope: EventName and EventFields (FieldName/FieldType/FieldValue).
// OSConfig keeps its existing properties string-valued. No trailing NUL is sent.
// Product event definitions and common census data belong to this adapter.
int TelemetryPackEvent(const char* name, const TelemetryProperty* properties, size_t count,
    unsigned char* payload, size_t capacity, size_t* size, OsConfigLogHandle log);

#ifdef __cplusplus
}
#endif

#endif // EVENT_PAYLOAD_H
