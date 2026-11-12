// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Logging.h>

#define TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR "activityId"

#ifdef __cplusplus
extern "C"
{
#endif

void TelemetryInitialize(const OsConfigLogHandle log);
void TelemetryCleanup(OsConfigLogHandle log);
void OSConfigTelemetryBaselineRun(const char* baseline, const char* mode, double seconds);
void OSConfigTelemetryCrashDetected(const char* crashInfo);

#ifdef __cplusplus
}
#endif

#endif
