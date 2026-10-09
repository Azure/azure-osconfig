// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef EVENT_H
#define EVENT_H

#include "Encoder.h"

#define TELEMETRY_CLIENT_VERSION "OSConfig-C/0.1"
#define TELEMETRY_EPOCH_SIZE 37

#ifdef __cplusplus
extern "C"
{
#endif

// Private IPC payload: native uint32 count, then NUL-terminated event name and
// count name/value string pairs. No pointers, JSON escaping, or event storage.
int TelemetryPackEvent(const char* name, const TelemetryProperty* properties, size_t count, unsigned char* payload, size_t capacity, size_t* size, OsConfigLogHandle log);
int TelemetryEncodePayload(const unsigned char* payload, size_t size, const char* iKey, const char* epoch, int64_t sequence,
    unsigned char* bytes, size_t* encodedSize, int64_t* uploadTime, OsConfigLogHandle log);
int TelemetryCreateEpoch(char* epoch, OsConfigLogHandle log);

#ifdef __cplusplus
}
#endif

#endif
