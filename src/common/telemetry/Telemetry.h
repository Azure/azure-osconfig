// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TELEMETRY_H
#define TELEMETRY_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <Logging.h>
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
    return (int64_t)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}

#ifdef BUILD_TELEMETRY
// Serialized invocation scope. Reinitialization returns EALREADY without
// resetting its deadlines. The caller keeps log valid until cleanup.
int TelemetryInitialize(OsConfigLogHandle log);
void TelemetryCleanup(OsConfigLogHandle log);
char* GetModuleDirectory(void);
char* GetCachedDistroName(void);
void OSConfigTimeStampSave(void);
void OSConfigGetElapsedTime(int64_t* microseconds);
void TelemetryStatusTrace(const char* callingFunction, int status, const char* file,
    const char* function, int line);
void OSConfigTelemetryBaselineRun(const char* baseline, const char* mode, double seconds);
void OSConfigTelemetryRuleComplete(const char* component, const char* object, int result, int64_t microseconds);
void OSConfigTelemetryCrashDetected(const char* crashInfo);

#define OSConfigTelemetryStatusTrace(callingFunction, status) \
    TelemetryStatusTrace((callingFunction), (status), __FILE__, __func__, __LINE__)
#define OSConfigTelemetryStatusTraceImpl(callingFunction, status, line) \
    TelemetryStatusTrace((callingFunction), (status), __FILE__, __func__, (line))
#else
static inline int TelemetryInitialize(OsConfigLogHandle log) { (void)log; return 0; }
static inline void TelemetryCleanup(OsConfigLogHandle log) { (void)log; }
static inline char* GetModuleDirectory(void) { return NULL; }
static inline char* GetCachedDistroName(void) { return NULL; }
static inline void OSConfigTimeStampSave(void) {}
static inline void OSConfigGetElapsedTime(int64_t* value) { if (value) *value = 0; }
#define OSConfigTelemetryStatusTrace(callingFunction, status) \
    do { (void)(callingFunction); (void)(status); } while (0)
#define OSConfigTelemetryStatusTraceImpl(callingFunction, status, line) \
    do { (void)(callingFunction); (void)(status); (void)(line); } while (0)
#define OSConfigTelemetryBaselineRun(baseline, mode, seconds) \
    do { (void)(baseline); (void)(mode); (void)(seconds); } while (0)
#define OSConfigTelemetryRuleComplete(component, object, result, microseconds) \
    do { (void)(component); (void)(object); (void)(result); (void)(microseconds); } while (0)
#define OSConfigTelemetryCrashDetected(crashInfo) do { (void)(crashInfo); } while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif
