// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef IPC_INTERFACE_H
#define IPC_INTERFACE_H

#include "Resolver.h"
#include "SchemaEncoder.h"
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
// A lookup failure does not require a new worker. Output is cleared on failure.
int TelemetryWorkerResolve(TelemetryWorker* worker, const char* host, TelemetryResolvedHost* result, OsConfigLogHandle log);

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
