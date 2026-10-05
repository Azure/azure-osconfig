// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_WORKER_H
#define TELEMETRY_WORKER_H

#include "TelemetryResolver.h"
#include "TelemetryEncoder.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct TelemetryWorker TelemetryWorker;

// All calls are serialized. The host must not reap our child or change SIGCHLD
// handling while it is owned. workerPath is a trusted absolute executable path.
// *worker must initially be NULL. Creation does not spawn or perform network I/O.
// Lifetime includes audit time; the telemetry budget charges only API execution.
// Production passes forceMinTls=false; tests can retain forced mintls across
// child restarts without changing deadlines or the selected route.
int TelemetryWorkerCreate(const char* workerPath, int lifetimeMilliseconds,
    int budgetMilliseconds, int operationMilliseconds, TelemetryWorker** worker,
    bool forceMinTls, OsConfigLogHandle log);

// Starts the worker lazily and reuses it. IPC/process failure drops this operation
// and reaps the child; a later call may start another within the ORIGINAL budget.
// A lookup failure does not require a new worker. Output is cleared on failure.
int TelemetryWorkerResolve(TelemetryWorker* worker, const char* host,
    TelemetryResolvedHost* result, OsConfigLogHandle log);

// Synchronously sends one named event. Properties are borrowed strings. Packing,
// startup, encoding, network, and acknowledgment share the operation budget.
// Nonzero means dropped/rejected/unconfirmed, never queued or replayed.
int TelemetryWorkerSend(TelemetryWorker* worker, const char* name,
    const TelemetryProperty* properties, size_t count, OsConfigLogHandle log);

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

#endif // TELEMETRY_WORKER_H
