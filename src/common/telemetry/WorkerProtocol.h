// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef WORKER_PROTOCOL_H
#define WORKER_PROTOCOL_H

#include <stdint.h>

#define TELEMETRY_WORKER_MAGIC UINT32_C(0x4F534354)
#define TELEMETRY_WORKER_VERSION 1
#define TELEMETRY_WORKER_READY 1
#define TELEMETRY_WORKER_RESOLVE 2
#define TELEMETRY_WORKER_SEND 3
#define TELEMETRY_WORKER_ARGUMENT "--ipc-v1"

// Private native-endian protocol: the SO and worker must come from one package.
// One outstanding request, no event queue. Deadlines are absolute monotonic ns.
// Replies have 0 == deadline; a nonzero status has no body.
typedef struct TelemetryWorkerFrame
{
    uint32_t magic;
    uint32_t version;
    uint32_t operation;
    uint32_t size;
    uint32_t sequence;
    int32_t status;
    int64_t deadline;
} TelemetryWorkerFrame;

// SEND reports event outcome separately from IPC failure. Suppression is latched
// in the parent too, so replacing a failed child cannot bypass collector controls.
typedef struct TelemetryWorkerSendReply
{
    int32_t status;
    uint32_t suppressed;
} TelemetryWorkerSendReply;

#endif // WORKER_PROTOCOL_H
