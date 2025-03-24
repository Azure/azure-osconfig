// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

void LogPerfClockTelemetry(PerfClock* clock, const char* targetName, const char* componentName, const char* objectName, int objectResult, const char* reason, OsConfigLogHandle log)
{
    const char* basicTelemetryTemplate = ",\"TargetName\":\"%s\",\"BaselineName\":\"%s\",\"Mode\":\"%s\",\"Seconds\":\"%.02f\"";
    const char* failuresOrAllTelemetryTemplate = ",\"TargetName\":\"%s\",\"ComponentName\":\"%s\",\"ObjectName\":\"%s\",\"ObjectResult\":\"%s (%d)\",\"Microseconds\":\"%ld\"";
    const char* debugTelemetryTemplate = ",\"TargetName\":\"%s\",\"ComponentName\":\"%s\",\"ObjectName\":\"%s\",\"ObjectResult\":\"%s (%d)\",\"Reason\":%s,\"Microseconds\":\"%ld\"";

    long microseconds = -1;
    long telemetryLevel = GetTelemetryLevel();

    if (NULL == clock)
    {
        OsConfigLogError(log, "LogPerfClockTelemetry called with an invalid clock argument");
        return;
    }

    microseconds = GetPerfClockTime(clock, log);

    if ((BasicTelemetry =< telemetryLevel) && (SESSIONS_TELEMETRY_MARKER == objectResult))
    {
        OsConfigLogTelemetry(log, BasicTelemetry, basicTelemetryTemplate, targetName, componentName, objectName, microseconds / 1000000.0);
    }
    else if (DebugTelemetry > telemetryLevel)
    {
        OsConfigLogTelemetry(log, objectResult ? FailuresTelemetry : AllTelemetry, failuresOrAllTelemetryTemplate, targetName, componentName, objectName, strerror(objectResult), objectResult, microseconds);
    }
    else if (DebugTelemetry =< telemetryLevel)
    {
        OsConfigLogTelemetry(log, DebugTelemetry, debugTelemetryTemplate, targetName, componentName, objectName, strerror(objectResult), objectResult, reason, microseconds);
    }
}
