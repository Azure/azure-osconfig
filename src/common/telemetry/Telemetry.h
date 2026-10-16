// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_H
#define TELEMETRY_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <Logging.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define TELEMETRY_BINARY_NAME "OSConfigTelemetry"
#define TELEMETRY_NOTFOUND_STRING "N/A"
#define TELEMETRY_CORRELATIONID_ENVIRONMENT_VAR "activityId"
#define TELEMETRY_RULECODENAME_ENVIRONMENT_VAR "_RuleCodename"
#define TELEMETRY_SCENARIONAME_ENVIRONMENT_VAR "_ScenarioName"
#define TELEMETRY_MICROSECONDS_ENVIRONMENT_VAR "_Microseconds"

#ifdef __cplusplus
extern "C"
{
#endif

static inline int64_t TsToUs(struct timespec ts)
{
    return (((int64_t)ts.tv_sec * 1000000LL) + (ts.tv_nsec / 1000));
}

// Serialized invocation scope. Failures are logged internally; callers retain
// the initialize/emit/cleanup contract and keep log valid until cleanup.
void TelemetryInitialize(const OsConfigLogHandle log);
void TelemetryCleanup(OsConfigLogHandle log);
char* GetModuleDirectory(void);
char* GetCachedDistroName(void);
void OSConfigTimeStampSave(void);
void OSConfigGetElapsedTime(int64_t* microseconds);
void OSConfigTelemetryStatusTraceInternal(const char* callingFunction, int status, const char* file, const char* function, int line);
void OSConfigTelemetryBaselineRun(const char* baseline, const char* mode, double seconds);
void OSConfigTelemetryRuleComplete(const char* component, const char* object, int result, int64_t microseconds);
void OSConfigTelemetryCrashDetected(const char* crashInfo);

#define OSConfigTelemetryStatusTrace(callingFunction, status) \
    OSConfigTelemetryStatusTraceInternal((callingFunction), (status), __FILE__, __func__, __LINE__)
#define OSConfigTelemetryStatusTraceImpl(callingFunction, status, line) \
    OSConfigTelemetryStatusTraceInternal((callingFunction), (status), __FILE__, __func__, (line))

#ifdef __cplusplus
}
#endif

#endif
