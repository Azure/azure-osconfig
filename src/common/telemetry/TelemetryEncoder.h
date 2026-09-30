// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_ENCODER_H
#define TELEMETRY_ENCODER_H

#include <Logging.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TELEMETRY_MAX_EVENT_SIZE 16384
#define TELEMETRY_MAX_PROPERTY_COUNT 32
#define TELEMETRY_MAX_NAME_LENGTH 100

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

typedef struct TelemetryEvent
{
    const char* name;
    const char* iKey;
    // Common Schema UTC ticks since year 1, not Unix milliseconds.
    int64_t time;
    int64_t flags;
    // Optional, already normalized Common Schema device.localId.
    const char* deviceId;
    const char* sdkVersion;
    const char* sdkEpoch;
    int64_t sequence;
    const TelemetryProperty* properties;
    size_t propertyCount;
} TelemetryEvent;

// Encodes one uncompressed Common Schema 3 custom event, without an HTTP envelope.
// Inputs are borrowed, NUL-terminated UTF-8 strings and must not overlap outputs
// or change during the call. Optional metadata strings can be NULL.
// Returns zero or a logged errno value. On failure, *encodedSize is zero and
// buffer is unchanged. Production callers must supply their valid log handle.
// This initial subset has no property privacy attributes or OS/app extensions;
// it must not be used to discard such metadata from a richer event.
int TelemetryEncodeEvent(const TelemetryEvent* event, unsigned char* buffer,
    size_t capacity, size_t* encodedSize, OsConfigLogHandle log);

#ifdef __cplusplus
}
#endif

#endif // TELEMETRY_ENCODER_H
