// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef IPC_INTERFACE_H
#define IPC_INTERFACE_H

#include "Utils.h"
#include "EventPayload.h"
#include <stdint.h>

#define TELEMETRY_WORKER_MAGIC UINT32_C(0x4F534354)
#define TELEMETRY_WORKER_VERSION 3
#define TELEMETRY_WORKER_READY 1
#define TELEMETRY_WORKER_SEND 3
#define TELEMETRY_WORKER_ARGUMENT "--pipe-v3"
#define TELEMETRY_WORKER_HEADER_SIZE 32

// Pipe v3: explicit little-endian header, UTF-8 JSON SEND payload.
// deadline is a relative timeout in milliseconds (1..500), zero in replies.
// One outstanding request, no replay. Status values are portable, not errno.
typedef struct TelemetryWorkerFrame
{
    uint32_t magic;
    uint32_t version;
    uint32_t operation;
    uint32_t size;
    uint32_t sequence;
    uint32_t status;
    uint64_t deadline;
} TelemetryWorkerFrame;

// SEND reports event outcome separately from IPC failure. Suppression is latched
// in the parent too, so replacing a failed child cannot bypass collector controls.
typedef struct TelemetryWorkerSendReply
{
    uint32_t status;
    uint32_t suppressed;
} TelemetryWorkerSendReply;

static inline void TelemetryWriteWord(unsigned char* bytes, uint32_t value)
{
    unsigned int i;
    for (i = 0; i < 4; ++i)
    {
        bytes[i] = (unsigned char)(value >> (8 * i));
    }
}

static inline uint32_t TelemetryReadWord(const unsigned char* bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
        ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static inline void TelemetryEncodeFrame(const TelemetryWorkerFrame* frame, unsigned char* bytes)
{
    TelemetryWriteWord(bytes, frame->magic);
    TelemetryWriteWord(bytes + 4, frame->version);
    TelemetryWriteWord(bytes + 8, frame->operation);
    TelemetryWriteWord(bytes + 12, frame->size);
    TelemetryWriteWord(bytes + 16, frame->sequence);
    TelemetryWriteWord(bytes + 20, frame->status);
    TelemetryWriteWord(bytes + 24, (uint32_t)frame->deadline);
    TelemetryWriteWord(bytes + 28, (uint32_t)(frame->deadline >> 32));
}

static inline void TelemetryDecodeFrame(const unsigned char* bytes, TelemetryWorkerFrame* frame)
{
    frame->magic = TelemetryReadWord(bytes);
    frame->version = TelemetryReadWord(bytes + 4);
    frame->operation = TelemetryReadWord(bytes + 8);
    frame->size = TelemetryReadWord(bytes + 12);
    frame->sequence = TelemetryReadWord(bytes + 16);
    frame->status = TelemetryReadWord(bytes + 20);
    frame->deadline = TelemetryReadWord(bytes + 24) | ((uint64_t)TelemetryReadWord(bytes + 28) << 32);
}

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct TelemetryWorker TelemetryWorker;

// Private status-returning hook for fixtures.
// Production callers use the unchanged TelemetryInitialize(log) API.
int TelemetryInitializeInternal(OsConfigLogHandle log);

// All calls are serialized. The host must not reap our child or change SIGCHLD
// handling while it is owned. workerPath is a trusted absolute executable path.
// *worker must initially be NULL. Creation does not spawn or perform network I/O.
// Lifetime includes audit time; the telemetry budget charges only API execution.
// Permanent startup/READY failures are retained until this invocation is destroyed.
int TelemetryWorkerCreate(const char* workerPath, int lifetimeMilliseconds, int budgetMilliseconds, int operationMilliseconds, TelemetryWorker** worker, OsConfigLogHandle log);

// Starts the worker lazily and reuses it. IPC/process failure drops this operation
// and reaps the child; a later call may start another within the ORIGINAL budget
// unless a permanent startup failure has disabled this invocation.
// Synchronously sends one named event. Properties are borrowed strings. Packing,
// startup, encoding, network, and acknowledgment share the operation budget.
// Nonzero means dropped/rejected/unconfirmed, never queued or replayed.
int TelemetryWorkerSendEvent(TelemetryWorker* worker, const char* name, const TelemetryProperty* properties, size_t count, OsConfigLogHandle log);

// Charge producer-side metadata preparation before beginning the SEND deadline.
// started is a monotonic nanosecond timestamp from immediately before that work.
int TelemetryWorkerAccountPreparation(TelemetryWorker* worker, int64_t started, OsConfigLogHandle log);

// Idempotent for NULL == *worker. Always closes IPC and handles the exact child.
// If child ownership cannot be discharged, retains *worker for cleanup retry.
// Termination/reaping can exceed deadlines if the kernel cannot make progress.
int TelemetryWorkerDestroy(TelemetryWorker** worker, OsConfigLogHandle log);

#ifdef __cplusplus
}
#endif

#endif // IPC_INTERFACE_H
